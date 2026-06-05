#include "Graph.h"

#include "../FibHeap/FibHeap.h"

/*!
 * \defgroup techdetails Technical Details
 * \brief Detailed technical information for developers.
 * @{
 */

/** Intermediate DOT source emitted before rendering. */
#define DOT_SOURCE_PATH "fib_heap.dot"
/** Rendered PNG produced from the DOT source. */
#define DOT_IMAGE_PATH "fib_heap.png"
/** Graphviz command that turns the DOT source into the PNG. */
#define DOT_RENDER_CMD "dot -Tpng " DOT_SOURCE_PATH " -o " DOT_IMAGE_PATH

/**
 * @brief Recursively emit a sibling ring and every subtree hanging off it.
 *
 * @param heap The heap being visualized (used to highlight the minimum).
 * @param ring Any node of the circular sibling list to render.
 * @param file Destination DOT stream.
 */
static void writeRing(FibHeap* heap, FibNode* ring, FILE* file);

/**
 * @brief Write the graph preamble (title, layout and default node shape).
 */
static void writeHeader(FILE* file) {
    fprintf(file,
            "digraph G {\n"
            "labelloc=\"t\"\n"
            "label=\"ASM Fibonacci heap by KXI\"\n"
            "rankdir=TB;\n"
            "node [shape=box];\n");
}

/**
 * @brief Declare a single node and its blue/green links to its siblings.
 *
 * The heap minimum is additionally colored red.
 */
static void writeNode(FibHeap* heap, FibNode* node, FILE* file) {
    fprintf(file, "n%p [label=\"key=%ld\naddr=%p\"];\n",
            (void*)node, node->key, (void*)node);

    if (node == heap->min) {
        fprintf(file, "n%p [color=red];\n", (void*)node);
    }

    fprintf(file, "n%p -> n%p [color=blue];\n", (void*)node, (void*)node->right);
    fprintf(file, "n%p -> n%p [color=green];\n", (void*)node, (void*)node->left);
}

/**
 * @brief Emit one sibling ring as a single rank ("row") in the layout.
 */
static void writeRankRow(FibHeap* heap, FibNode* ring, FILE* file) {
    fprintf(file, "{ rank=same; ");

    FibNode* node = ring;
    do {
        writeNode(heap, node, file);
        node = node->right;
    } while (node != ring);

    fprintf(file, "}\n");
}

/**
 * @brief Emit the parent->child links of a ring and recurse into each child.
 */
static void writeChildLinks(FibHeap* heap, FibNode* ring, FILE* file) {
    FibNode* node = ring;
    do {
        if (node->child != NULL) {
            fprintf(file, "n%p -> n%p [color=black];\n",
                    (void*)node, (void*)node->child);
            writeRing(heap, node->child, file);
        }
        node = node->right;
    } while (node != ring);
}

static void writeRing(FibHeap* heap, FibNode* ring, FILE* file) {
    writeRankRow(heap, ring, file);
    writeChildLinks(heap, ring, file);
}

void generateFibHeapDot(FibHeap* heap) {
    FILE* file = fopen(DOT_SOURCE_PATH, "w");
    if (file == NULL) {
        perror("Unable to open " DOT_SOURCE_PATH);
        return;
    }

    writeHeader(file);
    if (heap->min != NULL) {
        writeRing(heap, heap->min, file);
    }
    fprintf(file, "}\n");

    fclose(file);

    (void)system(DOT_RENDER_CMD);
}

/*! @} */
