extern calloc
extern free

extern stackCtor
extern stackDtor
extern stackPush
extern stackPop
extern stackTop
extern stackSize

global ASM_fibHeapCtor
global ASM_fibHeapInit
global ASM_fibHeapDtor
global ASM_fibHeapIns
global ASM_fibHeapDel
global ASM_fibHeapGetMin
global ASM_fibHeapExtMin
global ASM_fibHeapMerge
global ASM_fibHeapGetSize
global ASM_fibHeapOverrideKey

;-------------------------
; STRUCT: FibHeap
;
; For a detailed description of this structure, 
; see the h file, there will be offsets
;
; offset to FibNode* min
fh_min_offset: equ 0
; offset to uint64_t size
fh_size_offset: equ 8
; offset to FibNode** array
fh_array_offset: equ 16
;
;-------------------------

;-------------------------
; STRUCT: FibNode
;
; For a detailed description of this structure, 
; see the h file, there will be offsets
;
; offset to Key_t key
fn_key_offset:      equ 0
; offset to FibNode* parent
fn_parent_offset:   equ 8
; offset to FibNode* left
fn_left_offset:     equ 16
; offset to FibNode* right
fn_right_offset:    equ 24
; offset to FibNode child
fn_child_offset:    equ 32
; offset to uint64_t degree
fn_degree_offset:   equ 40
; offset to uint8_t
fn_mark_offset:     equ 48
;
; 7 more bytes of alignment
;-------------------------


;-------------------------
;
; Constants
;
size_nodes_arr: equ 50000
sizeof_FibHeap: equ 24
sizeof_FibNode: equ 56
;-------------------------

section .text

;-------------------------
;
; FUNC: ASM_fibHeapCtor
; FibHeap* ASM_fibHeapCtor(void)
;
; INPUT : NONE (void)
; OUTPUT: rax = fibHeap pointer
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapCtor:
    push rbx ; Storing registers recovery

    mov rdi, 1               ; rdi = 1 = number of fibHeaps
    mov rsi, sizeof_FibHeap  ; rsi = 24 = sizeof(FibHeap)
    call calloc WRT ..plt    ; rax = calloc(1, sizeof(FibHeap))


    test rax, rax; if(rax == NULL)
    jz .exit     ; return NULL


    mov rbx, rax ; rbx = heap


    mov rdi, size_nodes_arr             ; rdi = ASMFIBARRSTARTSIZE = 50000
    mov rsi, 8                          ; rsi = sizeof(FibNode*) = 8
    call calloc WRT ..plt               ; rax = calloc(50000, sizeof(FibNode*))
    test rax, rax                       ; if(rax == NULL)
    jnz .if_1_end                       ; {
        mov rdi, rbx                    ; rdi = rax = heap
        call free WRT ..plt             ; free(heap)
        jmp .exit                       ; return;
    .if_1_end:                          ; }
    mov [rbx + fh_array_offset], rax    ; heap->array = calloc(...)
    mov rax, rbx                        ; rax = heap


    .exit:
    pop rbx ; Restoring registers


    ret ; return rax = heap


;-------------------------
;
; FUNC: ASM_fibNodeInit
; static inline FibNode* ASM_fibNodeInit(Key_t key)
;
; INPUT: RDI = Key_t key
; OUTPUT: rax = fibNode poiner
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibNodeInit:
    push rdi ; push key


    mov rdi, 1               ; rdi = 1
    mov rsi, sizeof_FibNode  ; rsi = 56 = sizeof(FibNode)
    call calloc WRT ..plt    ; rax = calloc(1, sizeof(FibNode))


    test rax, rax   ; if(rax == NULL)
    jz .exit        ; reutrn NULL

    pop rdi                          ; rdi = key
    mov [rax + fn_key_offset], rdi   ; node->key = key
    mov [rax + fn_left_offset], rax  ; node->left = node
    mov [rax + fn_right_offset], rax ; node->right = node
    

    .exit:
    ret ; return rax(node)

;-------------------------
;
; FUNC: ASM_fibHeapInit
; FibHeap* ASM_fibHeapInit(Key_t key)
;
; INPUT: RDI = Key_t key
; OUTPUT: rax = fibHeap poiner
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapInit:
    call ASM_fibNodeInit                ; rax = fibNodeInit(key)
    push rax                            ; push node
    call ASM_fibHeapCtor                ; rax = fibHeapCtor
    pop rdx                             ; rdx = node
    mov [rax + fh_min_offset], rdx      ; heap->min = fibNodeInit(key)
    mov qword [rax + fh_size_offset], 1 ; heap->size = 1
    ret                                 ; return heap

;-------------------------
;
; FUNC: ASM_fibHeapDtor
; void ASM_fibHeapDtor(FibHeap* heap)
;
; INPUT: rdi = heap
; OUTPUT: NONE
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapDtor:
    push rdi                        ; push heap
    mov rdi, [rdi + fh_min_offset]  ; rdi = heap->min
    call ASM_fibNodeDtor            ; free(heap->min)


    pop rdi                          ; rdi = heap
    push rdi                         ; push heap
    mov rdi, [rdi + fh_array_offset] ; rdi = heap->array
    call free WRT ..plt              ; free(heap->array)


    pop rdi             ; rdi = heap
    call free WRT ..plt ; free(heap)


    ret

;-------------------------
;
; FUNC: ASM_fibHeapIns
; FibNode* ASM_fibHeapIns(FibHeap* heap, Key_t key) 
;
; INPUT: rdi = heap, rsi = key
; OUTPUT: rax = new_node pointer
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapIns:
    push rbx ; saving register for recovery


    test rdi, rdi ; if(haep == NULL)
    jz .exit      ; return


    mov rbx, rdi ; rbx = heap


    mov rdi, rsi         ; rdi = key
    call ASM_fibNodeInit ; rax = new_node


    cmp qword [rbx + fh_size_offset], 0     ; if(heap->size == 0) {
    jne .if_1_else                          ;
        mov [rbx + fh_min_offset], rax      ; heap->min = rax = new_node
        jmp .if_1_end                       ;
    .if_1_else:                             ; } else {
        mov rsi, [rbx + fh_min_offset]      ; rsi = heap->min
        mov rdi, rax                        ; rdi = new_node
        call ASM_fibNodeUnionLists          ; fibNodeUnionLists(heap->min, new_node)


        mov rdx, [rbx + fh_min_offset]      ; rdx = heap->min
        mov rdx, [rdx + fn_key_offset]      ; rdx = heap->min->key
        cmp rdx, [rax + fn_key_offset]      ; if(heap->min->key > new_node->key)
        jle .if_2_end                       ; {
            mov [rbx + fh_min_offset], rax  ; heap->min = new_node;
        .if_2_end:                          ; }
    .if_1_end:                              ; }


    inc qword [rbx + fh_size_offset]    ; ++heap->size


    .exit:
    pop rbx ; restore spoiled register
    ret
;-------------------------


;-------------------------
;
; FUNC: ASM_fibHeapGetMin
; FibNode* ASM_fibHeapGetMin(const FibHeap* heap)
;
; INPUT: rdi = heap
; OUTPUT: rax = min_node
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapGetMin:
    mov rax, [rdi + fh_min_offset]  ; rax = heap->min
    ret


;-------------------------
;
; FUNC: ASM_fibHeapExtMin
; void ASM_fibHeapExtMin(FibHeap* heap)
;
; INPUT: rdi = heap
; OUTPUT: NONE
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapExtMin:
    push rbx ; saving register for recovery


    test rdi, rdi   ; if(heap == NULL)
    jz .exit        ; return


    mov rbx, rdi ; rbx = heap


    cmp qword [rbx + fh_size_offset], 0 ; if(heap->size == 0)
    je .exit                            ; return


    cmp qword [rbx + fh_size_offset], 1     ; if(heap->size == 1)
    jne .if_1_end                           ; {
        mov rdi, [rbx + fh_min_offset]      ; rdi = heap->min
        call free WRT ..plt                 ; free(heap->min)
        mov qword [rbx + fh_min_offset], 0  ; heap->min = 0
        mov qword [rbx + fh_size_offset], 0 ; heap->size = 0
        jmp .exit                           ; return
    .if_1_end:                              ; }


    push qword [rbx + fh_min_offset] ; push heap->min


    mov rdi, [rbx + fh_min_offset]       ; rdi = heap->min
    cmp qword [rdi + fn_child_offset], 0 ; if(heap->min->child != NULL)
    je .if_2_end                         ; {
        mov rsi, [rdi + fn_child_offset] ; rsi = heap->min->child
        call ASM_fibNodeUnionLists       ; ASM_fibNodeUnionLists(heap->min, heap->min->child)
    .if_2_end:                           ; }


    mov rdi, [rbx + fh_min_offset]  ; rdi = heap->min
    call ASM_fibNodeUntie           ; fibNodeUntie(heap->min)


    mov rax, [rbx + fh_min_offset]   ; rax = heap->min
    mov rax, [rax + fn_right_offset] ; rax = heap->min->right
    mov [rbx + fh_min_offset], rax   ; heap->min = heap->min->right


    pop rdi             ; 
    call free WRT ..plt ; free(delete_node)


    dec qword [rbx + fh_size_offset] ; --heap->size;


    mov rdi, rbx                ; rdi = heap
    call ASM_fibHeapConsolidate ; fibHeapConsolidate(heap)


    .exit:
    pop rbx ; restore spoiled register
    ret     ; return

;-------------------------
;
; FUNC: ASM_fibNodeDtor
; static inline void fibNodeDtor(FibNode* node)
;
; INPUT: rdi = node
; OUTPUT: NONE
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibNodeDtor:
    push rbx ; saving register for recovery
    push rbp ;
    push r12 ;


    test rdi, rdi   ; if(node == NULL)
    jz .exit        ; return


    mov rbx, rdi ; rbx = node


    call stackCtor  ; rax = stk = stackCtor
    test rax, rax   ; if(stk == NULL)
    jz .exit        ; return


    mov rbp, rax ; rbp = stk


    mov r12, rbx ; r12 = node = first_node


    .for_start: ; for(;;) {


        cmp qword [rbx + fn_child_offset], 0    ; if(node->child != NULL)
        je .if_1_end                            ; {
            mov rdi, rbp                        ; rdi = stk
            mov rsi, [rbx + fn_child_offset]    ; rsi = node->child
            call stackPush                      ; stackPush(stk, node->child)
        .if_1_end:                              ; }


        mov rdi, rbp                     ; rdi = stk
        mov rsi, [rbx + fn_right_offset] ; rsi = node->right
        call stackPush                   ; stackPush(stk, node->right)


        mov rdi, rbx        ; rdi = node
        call free WRT ..plt ; free(node)


        mov rdi, rbp    ; rdi = stk
        call stackTop   ; rax = stackTop(stk)
        mov rbx, rax    ; rbx = rax = node


        mov rdi, rbp    ; rdi = stk
        call stackPop   ; stackPop(stk)


        cmp rbx, r12        ; if(node == first_node)
        jne .if_2_end       ; {
            mov rdi, rbp    ; rdi = stk
            call stackSize  ; rax = stackSize(stk)
            cmp rax, 0      ; if(stackSize(stk) == 0)
            je .for_end     ; break


            mov rdi, rbp    ; rdi = stk
            call stackTop   ; rax = stackTop(stk)
            mov rbx, rax    ; rbx = rax = node


            mov rdi, rbp    ; rdi = stk
            call stackPop   ; stackPop(stk)


            mov r12, rbx    ; first_node = node
        .if_2_end:          ; }


    jmp .for_start ; }
    .for_end:


    mov rdi, rbp    ; rdi = stk
    call stackDtor  ; stackDtor(stk)


    .exit:
    pop r12 ; restore spoiled register
    pop rbp ; 
    pop rbx ; 
    ret

;-------------------------
;
; FUNC: ASM_fibNodeUnionLists
; static inline void ASM_fibNodeUnionLists(FibNode* first, FibNode* second)
;
; INPUT: rdi = first, rsi = second
; OUTPUT: NONE
; SPOIL: rcx, rdx
;
;-------------------------
ASM_fibNodeUnionLists:
    test rdi, rdi   ; if(first == NULL)
    jz .exit        ; return
    test rsi, rsi   ; if(second == NULL)
    jz .exit        ; return 


    mov rdx, [rdi + fn_left_offset]  ; rdx = first->left
    mov rcx, [rsi + fn_right_offset] ; rcx = second->right


    mov [rdx + fn_right_offset], rcx ; first->left->right = second->right
    mov [rcx + fn_left_offset], rdx  ; second->right->left = first->left
    mov [rdi + fn_left_offset], rsi  ; first->left = second
    mov [rsi + fn_right_offset], rdi ; second->right = first


    .exit:
    ret


;-------------------------
;
; FUNC: ASM_fibNodeUntie
; static inline void ASM_fibNodeUntie(FibNode* node)
;
; INPUT: rdi = node
; OUTPUT: NONE
; SPOIL: rcx, rdx
;
;-------------------------
ASM_fibNodeUntie:
    mov rdx, [rdi + fn_left_offset]     ; rdx = node->left
    mov rcx, [rdi + fn_right_offset]    ; rcx = node->right
    mov [rdx + fn_right_offset], rcx    ; node->left->right = node->right
    mov [rcx + fn_left_offset], rdx     ; node->right->left = node->left
    ret

;-------------------------
;
; FUNC: ASM_fibHeapConsolidate
; static inline void ASM_fibHeapConsolidate(FibHeap* heap)
; 
; INPUT: rdi = heap
; OUTPUT: NONE
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapConsolidate:
    push rbx ; saving register for recovery
    push rbp ;
    push r12 ;
    push r13 ;


    test rdi, rdi   ; if(heap == NULL)
    jz .exit        ; return


    mov r12, rdi                        ; r12 = heap
    mov rbx, [rdi + fh_min_offset]      ; rbx = current_node = heap->min
    mov rbp, [rdi + fh_array_offset]    ; rbp = heap->array
    mov r13, rbx                        ; r13 = end_node = current_node


    .while_1_start: ; do {


        mov qword [rbx + fn_parent_offset], 0; current_node->parent = NULL


        mov rcx, [r12 + fh_min_offset]      ; rcx = heap->min
        mov rcx, [rcx + fn_key_offset]      ; rcx = heap->min->key
        cmp rcx, [rbx + fn_key_offset]      ; if (heap->min->key > current_node->key)
        jle .if_1_end                       ; {
            mov [r12 + fh_min_offset], rbx  ; heap->min = current_node
        .if_1_end:                          ; }


        mov r8, [rbx + fn_right_offset]    ; r8 = next_node = current_node->right


        mov rax, [rbx + fn_degree_offset]   ; rax = current_node->degree
        shl rax, 3                          ; rax *= 8 i.e. pointer
        .while_2_start:                     ;
        cmp qword [rbp + rax], 0            ; while(heap->array[currnet_node->degree] != NULL)
        je .while_2_end                     ; {


            mov rcx, [rbx + fn_key_offset]  ; rcx = current_node->key
            mov rdx, [rbp + rax]            ; rdx = heap->array[current_node->degree]
            cmp rcx, [rdx + fn_key_offset]  ; if(current_node->ket > heap->array[current_node->degree])
            jle .if_2_end                   ; {
                mov rcx, rbx                ; rcx = current
                mov rbx, rdx                ; rbx = heap->array[current_node->degree]
                mov [rbp + rax], rcx        ; heap->array[current_node->degree] = current
            .if_2_end:                      ; }

            
            mov rcx, qword [rbp + rax]  ; rcx = heap->array[current_node->degree]
            

            cmp r13, rcx                       ; if(end_node == heap->array[current_node->degree])
            cmove r13, [r13 + fn_right_offset] ; end_node = end_node->right;


            cmp r8, rcx                        ; if(next_node == heap->array[current_node->degree])
            cmove r8, [r8 + fn_right_offset]   ; next_node = next_node->right;


            cmp [r12 + fh_min_offset], rcx      ; if(heap->min == heap->array[current_node->degree])
            jne .if_3_end                       ; {
                mov [r12 + fh_min_offset], rbx  ; heap->min = current_node;
            .if_3_end:                          ; }


            mov rdi, rbx            ;
            mov rsi, rcx            ;
            call ASM_fibNodeLink    ; fibNodeLink(current_node, heap->array[current_node->degree]);


            mov qword [rbp + rax], 0 ; heap->array[current_node->degree] = NULL;


        add rax, 8         ; ++current_node->degree
        jmp .while_2_start ;
        .while_2_end:      ; }


        mov [rbp + rax], rbx; heap->array[currnet_node->degree] = currnet_node;
        mov rbx, r8         ; currnet_node = next_node;


    cmp rbx, r13        ; }
    jne .while_1_start  ; while(current_node != end_node);


    mov rbx, [r12 + fh_min_offset]          ; current_node = heap->min
    .while_3_start:                         ; do {
        mov rax, [rbx + fn_degree_offset]   ; rax = current_node->degree
        mov qword [rbp + 8 * rax], 0        ; heap->array[current_node->degree] = NULL
        mov rbx, [rbx + fn_right_offset]    ; current_node = current_node->right
    cmp rbx, [r12 + fh_min_offset]          ; }
    jne .while_3_start                      ; while(current_node != heap->min)


    .exit:
    pop r13 ; restore spoiled register
    pop r12 ;
    pop rbp ;
    pop rbx ;
    ret


;-------------------------
;
; FUNC: ASM_fibNodeLink
; static inline void ASM_fibNodeLink(FibNode* first, FibNode* second)
;
; INPUT: rdi = first, rsi = second
; OUTPUT: NONE
; SPOIL: rcx, rdx
;
;-------------------------
ASM_fibNodeLink:
    push rdi                ; push first
    mov rdi, rsi            ; rdi = second
    call ASM_fibNodeUntie   ; fibNodeUntie(second)
    call ASM_fibNodeFixate  ; fibNodeFixate(second)
    pop rdi                 ; rdi = first


    cmp qword [rdi + fn_child_offset], 0 ; if(first->child == NULL)
    jne .if_1_else                       ; {
        mov [rdi + fn_child_offset], rsi ; first->child = second
        jmp .if_1_end                    ;
    .if_1_else:                          ; } else { 
        push rdi                         ; push first
        mov rdi, [rdi + fn_child_offset] ; rdi = first->child
        call ASM_fibNodeUnionLists       ; fibNodeUnionLists(first->child, second)
        pop rdi                          ; rdi = first
    .if_1_end:                           ; }


    inc qword [rdi + fn_degree_offset]      ; first->degree++;
    mov [rsi + fn_parent_offset], rdi       ; second->parent = first


    ret ; return


;-------------------------
;
; FUNC: ASM_fibNodeFixate
; static inline void ASM_fibNodeFixate(FibNode* node)
;
; INPUT: rdi = node
; OUTPUT: NONE
; SPOIL: NONE
;
;-------------------------
ASM_fibNodeFixate:
    mov [rdi + fn_left_offset], rdi     ; node->left = node
    mov [rdi + fn_right_offset], rdi    ; node->right = node
    ret                                 ; return

;-------------------------
;
; FUNC: ASM_fibHeapDel
; void ASM_fibHeapDel(FibHeap* heap, FibNode* node)
;
; INPUT: rdi = heap, rsi = node
; OUTPUT: NONE
; SPOIL: rdx
;
;-------------------------
ASM_fibHeapDel:
    mov rdx, [rdi + fh_min_offset]  ;
    mov rdx, [rdx + fn_key_offset]  ;
    dec rdx                         ; rdx = new_key = heap->min->key - 1


    call ASM_fibHeapOverrideKey     ; fibHeapOverrideKey(heap, node, new_key)
    call ASM_fibHeapExtMin          ; fibHeapExtMin(heap);


    ret

;-------------------------
;
; FUNC: ASM_fibHeapOverrideKey
; void ASM_fibHeapOverrideKey(FibHeap* heap, FibNode* node, Key_t new_key)
;
; INPUT: rdi = heap, rsi = node, rdx = new_key
; OUTPUT: NONE
; SPOIL: rax, rcx, rdx
;
;-------------------------
ASM_fibHeapOverrideKey:
    mov [rsi + fn_key_offset], rdx  ; node->key = new_key


    mov rax, [rsi + fn_parent_offset]           ; rax = node->parent
    test rax, rax                               ; if(node->parent != NULL)
    jz .if_1_end                                ; {
        mov rcx, [rsi + fn_key_offset]          ; rcx = node->key
        cmp [rax + fn_key_offset], rcx          ; if(node->parent->key > node->key)
        jle .if_2_end                           ; {
            push rsi                            ; push node
            push qword [rsi + fn_parent_offset] ; push [node->parent]
            call ASM_fibHeapCut                 ; fibHeapCut(heap, node)
            pop rsi                             ; rsi = parent
            call ASM_fibHeapСascadingCut        ; fibHeapСascadingCut(heap, parent)
            pop rsi                             ; rsi = node
        .if_2_end:                              ; }
    .if_1_end:                                  ; }


    mov rax, [rdi + fh_min_offset]      ; rax = heap->min
    mov rax, [rax + fn_key_offset]      ; rax = heap->min->key
    cmp rax, [rsi + fn_key_offset]      ; if(heap->min->key > node->key) 
    jle .if_3_end                       ; {
        mov [rdi + fh_min_offset], rsi  ; heap->min = node
    .if_3_end:                          ; }


    ret

;-------------------------
;
; FUNC: ASM_fibHeapCut
; void ASM_fibHeapCut(FibHeap* heap, FibNode* node)
;
; INPUT: rdi = heap, rsi = node
; OUTPUT: NONE
; SPOIL: rcx, rdx
;
;-------------------------
ASM_fibHeapCut:
    mov byte [rsi + fn_mark_offset], 0 ; node->mark = false


    mov rdx, rsi                       ;
    mov rdx, [rdx + fn_parent_offset]  ; rdx = node->parent
    dec qword [rdx + fn_degree_offset] ; --node->parent->degree;


    cmp rsi, [rdx + fn_child_offset]              ; if(node->parent->child == node)
    jne .if_1_end                                 ; {
        cmp rsi, [rsi + fn_left_offset]           ; if(node == node->left)
        jne .if_2_else                            ; {
            mov qword [rdx + fn_child_offset], 0  ; node->parent->child = NULL
            jmp .if_2_end                         ;
        .if_2_else:                               ; } else {
            mov rcx, [rsi + fn_left_offset]       ; rcx = node->left
            mov [rdx + fn_child_offset], rcx      ; node->parent->child = node->left
        .if_2_end:                                ; }
    .if_1_end:


    push rdi                ;
    mov rdi, rsi            ; rdi = node
    call ASM_fibNodeUntie   ; fibNodeUntie(node)
    call ASM_fibNodeFixate  ; fibNodeFixate(node)
    pop rdi                 ;


    mov qword [rsi + fn_parent_offset], 0 ; node->parent = NULL


    push rdi                        ;
    mov rdi, [rdi + fh_min_offset]  ; rdi = heap->min
    call ASM_fibNodeUnionLists      ; fibNodeUnionLists(heap->min, node)
    pop rdi                         ;


    ret

;-------------------------
;
; FUNC: ASM_fibHeapСascadingCut
; void ASM_fibHeapСascadingCut(FibHeap* heap, FibNode* node)
;
; INPUT: rdi = heap, rsi = node
; OUTPUT: NONE
; SPOIL: rdx, rsi
;
;-------------------------
ASM_fibHeapСascadingCut:
    mov rdx, [rsi + fn_parent_offset]   ; rdx = parent = node->parent


    .while_start:                               ; while(parnet != NULL)
    test rdx, rdx                               ; {
    jz .while_end                               ;
        cmp byte [rsi + fn_mark_offset], 0      ; if(node->mark == false)
        jne .if_1_end                           ; {
            mov byte [rsi + fn_mark_offset], 1  ; node->mark = true
            jmp .while_end                      ; break
        .if_1_end:                              ; }
        call ASM_fibHeapCut                     ; fibHeapCut(heap, node)
        mov rsi, rdx                            ; node = parent
        mov rdx, [rsi + fn_parent_offset]       ; parent = node->parent
    .while_end:                                 ; }


    ret


;-------------------------
;
; FUNC: ASM_fibHeapGetSize
; uint64_t ASM_fibHeapGetSize(const FibHeap* heap)
;
; INPUT: rdi = heap
; OUTPUT: rax = heap->size
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapGetSize:
    mov rax, [rdi + fh_size_offset] ; rax = heap->size
    ret


;-------------------------
;
; FUNC: ASM_fibHeapMerge
; void ASM_fibHeapMerge(FibHeap* first, FibHeap* second)
;
; Mirrors the C fibHeapMerge: the second heap is spliced into the first
; and its heap struct is freed. Keeps the smaller of the two minima.
;
; INPUT: rdi = first, rsi = second
; OUTPUT: NONE
; SPOIL: according to the agreement
; caller saved registers
;
;-------------------------
ASM_fibHeapMerge:
    push rbx ; saving registers for recovery
    push r12 ;

    mov rbx, rdi ; rbx = first
    mov r12, rsi ; r12 = second


    cmp qword [rbx + fh_size_offset], 0     ; if(first->size == 0)
    jne .if_1_end                           ; {
        mov rax, [r12 + fh_min_offset]      ; rax = second->min
        mov [rbx + fh_min_offset], rax      ; first->min = second->min
        mov rax, [r12 + fh_size_offset]     ; rax = second->size
        mov [rbx + fh_size_offset], rax     ; first->size = second->size
        mov rdi, [r12 + fh_array_offset]    ; rdi = second->array
        call free WRT ..plt                 ; free(second->array)
        mov rdi, r12                        ; rdi = second
        call free WRT ..plt                 ; free(second)
        jmp .exit                           ; return
    .if_1_end:                              ; }


    cmp qword [r12 + fh_size_offset], 0     ; if(second->size == 0)
    jne .if_2_end                           ; {
        mov rdi, [r12 + fh_array_offset]    ; rdi = second->array
        call free WRT ..plt                 ; free(second->array)
        mov rdi, r12                        ; rdi = second
        call free WRT ..plt                 ; free(second)
        jmp .exit                           ; return
    .if_2_end:                              ; }


    mov rdi, [rbx + fh_min_offset]  ; rdi = first->min
    mov rsi, [r12 + fh_min_offset]  ; rsi = second->min
    call ASM_fibNodeUnionLists      ; fibNodeUnionLists(first->min, second->min)


    mov rax, [rbx + fh_min_offset]      ; rax = first->min
    mov rcx, [r12 + fh_min_offset]      ; rcx = second->min
    mov rdx, [rax + fn_key_offset]      ; rdx = first->min->key
    cmp rdx, [rcx + fn_key_offset]      ; if(first->min->key > second->min->key)
    jle .if_3_end                       ; {
        mov [rbx + fh_min_offset], rcx  ; first->min = second->min
    .if_3_end:                          ; }


    mov rax, [r12 + fh_size_offset]     ; rax = second->size
    add [rbx + fh_size_offset], rax     ; first->size += second->size


    mov rdi, [r12 + fh_array_offset]    ; rdi = second->array
    call free WRT ..plt                 ; free(second->array)
    mov rdi, r12                        ; rdi = second
    call free WRT ..plt                 ; free(second)


    .exit:
    pop r12 ; restore spoiled registers
    pop rbx ;
    ret     ; return


section .data