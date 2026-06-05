#include "FibHeap.h"

/**
 * @brief Swap the values of two variables.
 *
 * This macro swaps the values of two variables using a temporary variable.
 * It uses the `__typeof__` operator to ensure that the temporary variable
 * is of the same type as the input variables.
 *
 * @param a The first variable to be swapped.
 * @param b The second variable to be swapped.
 *
 * Example usage:
 * @code
 * int x = 5, y = 10;
 * swap(x, y);
 * // Now x is 10 and y is 5
 * @endcode
 * @warning a and b must be of the same type!
 */
#define swap(a, b)\
    do {\
        __typeof__ (a) temp = (a);\
        a = b;\
        b = temp;\
    } while (0)\

/*!
 * \defgroup techdetails Technical Details
 * \brief Detailed technical information for developers.
 * @{
 */

/**
 * @brief Create a new FibNode with an initialized element.
 * 
 * @param key The key for the new node.
 * @return A pointer to the newly created FibNode with an initial key.
 */
static inline FibNode* fibNodeInit(Key_t key);

/**
 * @brief Destroy a Fibonacci node and free its resources.
 *
 * @param node The node to be destroyed.
 */
static inline void fibNodeDtor(FibNode* node);

/**
 * @brief Union two FibNode lists.
 * 
 * @param first The first node to union.
 * @param second The second node to union.
 */
static inline void fibNodeUnionLists(FibNode* first, FibNode* second);

/**
 * @brief Consolidate the Fibonacci heap.
 * 
 * @param heap The Fibonacci heap to be consolidated.
 */
static inline void fibHeapConsolidate(FibHeap* heap);

/**
 * @brief Link the second node to the first node childs.
 * 
 * @param first The node to which they link.
 * @param second The node that is linked.
 */
static inline void fibNodeLink(FibNode* first, FibNode* second);

/**
 * @brief Untie the node from its list.
 * 
 * @param node The node to be untied.
 * @warning If there is only one node in the list, then a bug will occur
 */
static inline void fibNodeUntie(FibNode* node);

/**
 * @brief Loop the node on itself.
 * 
 * @param node The node that will be looped.
 */
static inline void fibNodeFixate(FibNode* node);

/**
 * @brief Procedure cutting node from its list and link to root list.
 * 
 * @param heap The heap from which the node will be cut and link into the root list.
 * @param node The node to be cut.
 */
static void fibHeapCut(FibHeap* heap, FibNode* node);

/**
 * @brief Perform cascading cut on a node in the Fibonacci heap.
 * 
 * @param The Fibonacci heap in which the cascading cut is performed.
 * @param node The starting node for the cascading cut.
 */
static void fibHeapСascadingCut(FibHeap* heap, FibNode* node);

FibHeap* fibHeapCtor(void) {
    FibHeap* heap = calloc(1, sizeof(FibHeap));
    assert(heap != NULL);

    heap->array = calloc(FIB_ARRAY_START_SIZE, sizeof(FibNode*));
    assert(heap->array != NULL);

    return heap;
}

FibHeap* fibHeapInit(Key_t key) {
    FibHeap* heap = fibHeapCtor();
    heap->min = fibNodeInit(key);
    heap->size = 1;
    return heap;
}

void fibHeapDtor(FibHeap* heap) {
    assert(heap != NULL);

    fibNodeDtor(heap->min);

    free(heap->array);
    free(heap);
}

FibNode* fibHeapIns(FibHeap* heap, Key_t key) {
    assert(heap != NULL);

    FibNode* new_node = fibNodeInit(key);
    if(heap->size == 0) {
        heap->min = new_node;
    } else {
        fibNodeUnionLists(heap->min, new_node);
        if(heap->min->key > new_node->key) {
            heap->min = new_node;
        }
    }
    ++heap->size;

    return new_node;
}

void fibHeapDel(FibHeap* heap, FibNode* node) {
    assert(heap != NULL);
    assert(node != NULL);

    Key_t new_key = heap->min->key - 1;

    fibHeapOverrideKey(heap, node, new_key);
    fibHeapExtMin(heap);
}

FibNode* fibHeapGetMin(const FibHeap* heap) {
    assert(heap != NULL);
    return heap->min;
}

void fibHeapExtMin(FibHeap* heap) {
    assert(heap != NULL);

    if(heap->size == 0) {
        return;
    }

    if(heap->size == 1) {
        free(heap->min);
        heap->min = NULL;
        heap->size = 0;
        return;
    }

    FibNode* delete_node = heap->min;

    if(heap->min->child != NULL) {
        fibNodeUnionLists(heap->min, heap->min->child);
    }

    fibNodeUntie(heap->min);
    heap->min = heap->min->right;

    free(delete_node);
    heap->size--;

    fibHeapConsolidate(heap);
}

void fibHeapMerge(FibHeap* first, FibHeap* second) {
    assert(first != NULL);
    assert(second != NULL);

    if(first->size == 0) {
        first->min = second->min;
        first->size = second->size;
        free(second->array);
        free(second);
        return;
    }

    if(second->size == 0) {
        free(second->array);
        free(second);
        return;
    }

    fibNodeUnionLists(first->min, second->min);

    if(first->min->key > second->min->key) {
        first->min = second->min;
    }

    first->size += second->size;

    free(second->array);
    free(second);
}

uint64_t fibHeapGetSize(const FibHeap* heap) {
    assert(heap != NULL);
    return heap->size;
}

void fibHeapOverrideKey(FibHeap* heap, FibNode* node, Key_t new_key) {
    assert(heap != NULL);
    assert(node != NULL);

    node->key = new_key;

    if(node->parent != NULL) {
        if(node->parent->key > node->key) {
            FibNode* parent = node->parent;
            fibHeapCut(heap, node);
            fibHeapСascadingCut(heap, parent);
        }
    }

    if(heap->min->key > node->key) {
        heap->min = node;
    }
}

static inline FibNode* fibNodeInit(Key_t key) {
    FibNode* node = calloc(1, sizeof(FibNode));
    assert(node != NULL);

    node->key = key;
    node->left = node;
    node->right = node;

    return node;
}

static inline void fibNodeDtor(FibNode* node) {
    if(node == NULL) {
        return;
    }

    Stack* stk = stackCtor();
    assert(stk != NULL);

    FibNode* first_node = node;

    for(;;) {
        if(node->child != NULL) {
            stackPush(stk, node->child);
        }

        stackPush(stk, node->right);
        free(node);
        node = stackTop(stk);
        stackPop(stk);

        if(node == first_node) {
            if(!stackSize(stk)) {
                break;
            }
            node = stackTop(stk);
            stackPop(stk);
            first_node = node;
        }
    }

    stackDtor(stk);
}

static inline void fibNodeUnionLists(FibNode* first, FibNode* second) {
    assert(first != NULL);
    assert(second != NULL);

    first->left->right = second->right;
    second->right->left = first->left;
    first->left = second;
    second->right = first;
}

static inline void fibHeapConsolidate(FibHeap* heap) {
    assert(heap != NULL);
    FibNode* currnet_node = heap->min;
    FibNode* end_node = currnet_node;
    do {
        currnet_node->parent = NULL;
        if(heap->min->key > currnet_node->key) {
            heap->min= currnet_node;
        }
        FibNode* next_node = currnet_node->right;
        while (heap->array[currnet_node->degree] != NULL) {
            if(currnet_node->key > heap->array[currnet_node->degree]->key) {
                swap(currnet_node, heap->array[currnet_node->degree]);
            }
            if(end_node == heap->array[currnet_node->degree]) {
                end_node = end_node->right;
            }
            if(next_node == heap->array[currnet_node->degree]) {
                next_node = next_node->right;
            }
            if(heap->min == heap->array[currnet_node->degree]) {
                heap->min = currnet_node;
            }
            fibNodeLink(currnet_node, heap->array[currnet_node->degree]);
            heap->array[currnet_node->degree - 1] = NULL;
        }
        heap->array[currnet_node->degree] = currnet_node;
        currnet_node = next_node;
    } while(currnet_node != end_node);

    currnet_node = heap->min;
    do {
        heap->array[currnet_node->degree] = NULL;
        currnet_node = currnet_node->right;
    } while(currnet_node != heap->min);
}

static inline void fibNodeLink(FibNode* first, FibNode* second) {
    fibNodeUntie(second);
    fibNodeFixate(second);

    if(first->child == NULL) {
        first->child = second;
    } else {
        fibNodeUnionLists(first->child, second);
    }

    first->degree++;

    second->parent = first;
}

static inline void fibNodeUntie(FibNode* node) {
    assert(node != NULL);

    node->left->right = node->right;
    node->right->left = node->left;
}

static inline void fibNodeFixate(FibNode* node) {
    assert(node != NULL);
    node->left = node;
    node->right = node;
}

static inline void fibHeapCut(FibHeap* heap, FibNode* node) {
    assert(heap != NULL);
    assert(node != NULL);
    assert(node->parent != NULL);

    node->mark = false;

    node->parent->degree--;

    if(node->parent->child == node) {
        if(node == node->left) {
            node->parent->child = NULL;
        } else {
            node->parent->child = node->left;
        }
    }

    fibNodeUntie(node);
    fibNodeFixate(node);

    node->parent = NULL;

    fibNodeUnionLists(heap->min, node);
}

static inline void fibHeapСascadingCut(FibHeap* heap, FibNode* node) {
    assert(heap != NULL);
    assert(node != NULL);

    FibNode* parent = node->parent;

    while(parent != NULL) {
        if(node->mark == false) {
            node->mark = true;
            break;
        }
        fibHeapCut(heap, node);
        node = parent;
        parent = node->parent;
    }
    node->mark = true;
}

/*! @} */
