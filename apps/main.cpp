// Interactive demo for the assembly Fibonacci heap.
//
// A small REPL that lets you build a heap by hand and watch it evolve. Every
// inserted key gets a short id (#1, #2, ...) so you can refer back to it for
// delete / decrease-key. When auto-draw is on (the default), the heap is
// re-rendered to fib_heap.png after each mutation via Graphviz.
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <unistd.h>  // isatty / STDIN_FILENO

extern "C" {
#include "ASM_FibHeap/ASM_FibHeap.h"
#include "Graph/Graph.h"
}

namespace {

// ASM_FibHeap and the C FibHeap are layout-identical (ASM_FibHeap.h is a
// byte-for-byte mirror of FibHeap.h), so the pure-C Graphviz exporter can
// render the assembly heap directly.
void drawHeap(ASM_FibHeap* heap) {
  generateFibHeapDot(reinterpret_cast<FibHeap*>(heap));
}

void printHelp() {
  std::cout <<
      "Commands (numeric aliases in parentheses match the batch protocol):\n"
      "  insert <key>          (i, 1)   insert a key, prints its id\n"
      "  extract               (e, 2)   remove and print the minimum\n"
      "  min                   (m)      show the minimum without removing it\n"
      "  delete <id>           (d, 3)   delete the node with that id\n"
      "  decrease <id> <key>   (c, 4)   lower an existing node's key\n"
      "  size                  (s)      number of elements in the heap\n"
      "  draw                  (g)      render the heap to fib_heap.png\n"
      "  autodraw <on|off>              toggle auto-render after each change\n"
      "  help                  (h, ?)   show this list\n"
      "  quit                  (q)      exit\n";
}

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

}  // namespace

int main() {
  const bool interactive = isatty(STDIN_FILENO) != 0;

  ASM_FibHeap* heap = ASM_fibHeapCtor();
  std::unordered_map<long, ASM_FibNode*> id_to_node;
  std::unordered_map<ASM_FibNode*, long> node_to_id;
  long next_id = 1;
  bool auto_draw = true;

  if (interactive) {
    std::cout << "ASM Fibonacci heap demo. Type 'help' for commands.\n";
  }

  std::string line;
  while (true) {
    if (interactive) {
      std::cout << "fib> " << std::flush;
    }
    if (!std::getline(std::cin, line)) {
      break;  // EOF
    }

    std::istringstream in(line);
    std::string cmd;
    if (!(in >> cmd)) {
      continue;  // blank line
    }
    cmd = lower(cmd);

    bool mutated = false;

    if (cmd == "insert" || cmd == "i" || cmd == "add" || cmd == "1") {
      int64_t key = 0;
      if (!(in >> key)) {
        std::cout << "usage: insert <key>\n";
        continue;
      }
      ASM_FibNode* node = ASM_fibHeapIns(heap, key);
      const long id = next_id++;
      id_to_node[id] = node;
      node_to_id[node] = id;
      std::cout << "inserted key=" << key << " as #" << id << "\n";
      mutated = true;

    } else if (cmd == "extract" || cmd == "extractmin" || cmd == "e" || cmd == "2") {
      if (ASM_fibHeapGetSize(heap) == 0) {
        std::cout << "heap is empty\n";
        continue;
      }
      ASM_FibNode* min = ASM_fibHeapGetMin(heap);
      std::cout << "extracted min key=" << min->key;
      auto it = node_to_id.find(min);
      if (it != node_to_id.end()) {
        std::cout << " (#" << it->second << ")";
        id_to_node.erase(it->second);
        node_to_id.erase(it);
      }
      std::cout << "\n";
      ASM_fibHeapExtMin(heap);
      mutated = true;

    } else if (cmd == "min" || cmd == "m" || cmd == "peek") {
      if (ASM_fibHeapGetSize(heap) == 0) {
        std::cout << "heap is empty\n";
        continue;
      }
      ASM_FibNode* min = ASM_fibHeapGetMin(heap);
      std::cout << "min key=" << min->key;
      auto it = node_to_id.find(min);
      if (it != node_to_id.end()) {
        std::cout << " (#" << it->second << ")";
      }
      std::cout << "\n";

    } else if (cmd == "delete" || cmd == "del" || cmd == "d" || cmd == "3") {
      long id = 0;
      if (!(in >> id)) {
        std::cout << "usage: delete <id>\n";
        continue;
      }
      auto it = id_to_node.find(id);
      if (it == id_to_node.end()) {
        std::cout << "no live node #" << id << "\n";
        continue;
      }
      ASM_FibNode* node = it->second;
      ASM_fibHeapDel(heap, node);
      node_to_id.erase(node);
      id_to_node.erase(it);
      std::cout << "deleted #" << id << "\n";
      mutated = true;

    } else if (cmd == "decrease" || cmd == "dec" || cmd == "c" || cmd == "4") {
      long id = 0;
      int64_t key = 0;
      if (!(in >> id >> key)) {
        std::cout << "usage: decrease <id> <key>\n";
        continue;
      }
      auto it = id_to_node.find(id);
      if (it == id_to_node.end()) {
        std::cout << "no live node #" << id << "\n";
        continue;
      }
      ASM_FibNode* node = it->second;
      if (key > node->key) {
        std::cout << "new key must be <= current key (" << node->key << ")\n";
        continue;
      }
      ASM_fibHeapOverrideKey(heap, node, key);
      std::cout << "decreased #" << id << " to key=" << key << "\n";
      mutated = true;

    } else if (cmd == "size" || cmd == "s") {
      std::cout << "size=" << ASM_fibHeapGetSize(heap) << "\n";

    } else if (cmd == "draw" || cmd == "graph" || cmd == "g" || cmd == "png") {
      drawHeap(heap);
      std::cout << "wrote fib_heap.png\n";

    } else if (cmd == "autodraw") {
      std::string arg;
      in >> arg;
      arg = lower(arg);
      if (arg == "on") {
        auto_draw = true;
      } else if (arg == "off") {
        auto_draw = false;
      } else {
        std::cout << "usage: autodraw <on|off>\n";
        continue;
      }
      std::cout << "auto-draw " << (auto_draw ? "on" : "off") << "\n";

    } else if (cmd == "help" || cmd == "h" || cmd == "?") {
      printHelp();

    } else if (cmd == "quit" || cmd == "exit" || cmd == "q") {
      break;

    } else {
      std::cout << "unknown command '" << cmd << "' — type 'help'\n";
    }

    if (mutated && auto_draw) {
      drawHeap(heap);
    }
  }

  ASM_fibHeapDtor(heap);
  return 0;
}
