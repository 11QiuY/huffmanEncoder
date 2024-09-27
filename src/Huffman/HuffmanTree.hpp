#include "minHBLT.hpp"
// #include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class HuffmanTree {
private:
  struct HuffmanNode {
    char ch;
    int freq;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(char ch, int freq)
        : ch(ch), freq(freq), left(nullptr), right(nullptr) {}
    bool operator<(const HuffmanNode &other) const { return freq < other.freq; }
    bool operator>(const HuffmanNode &other) const { return freq > other.freq; }
  };

public:
  std::shared_ptr<HuffmanNode>
  buildHuffmanTree(std::unordered_map<char, int> map) {
    minHBLT<HuffmanNode> minHeap;
    for (auto &pair : map) {
      minHeap.push(HuffmanNode(pair.first, pair.second));
    }
    while (minHeap.get_size() > 1) {
      std::shared_ptr<HuffmanNode> left = minHeap.top();
      minHeap.pop();
      std::shared_ptr<HuffmanNode> right = minHeap.top();
      minHeap.pop();
      std::shared_ptr<HuffmanNode> parent =
          std::make_shared<HuffmanNode>('\0', left->freq + right->freq);
      parent->left = left;
      parent->right = right;
      minHeap.push(parent);
    }
    return minHeap.top();
  }
  void HuffmanMap(std::shared_ptr<HuffmanNode> root,
                  std::unordered_map<char, std::string> &map,
                  std::string path) {
    if (root == nullptr) {
      return;
    }
    if (root->left == nullptr && root->right == nullptr) {
      map[root->ch] = path;
      return;
    }
    HuffmanMap(root->left, map, path + "0");
    HuffmanMap(root->right, map, path + "1");
  }

  void encode(const std::string &in, std::string &out,
              std::unordered_map<char, std::string> &map) {
    for (auto &c : in) {
      out += map[c];
    }
  }
};
