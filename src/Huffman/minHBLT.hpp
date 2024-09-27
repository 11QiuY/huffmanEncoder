// #include <iostream>
#include <memory>
#include <utility>
#include <vector>

template <typename T> class minHBLT {
  struct Node {
    std::pair<int, std::shared_ptr<T>> data;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    Node(const T da)
        : data(std::make_pair(1, std::make_shared<T>(std::move(da)))),
          left(nullptr), right(nullptr) {}
    Node(const std::shared_ptr<T> da)
        : data(std::make_pair(1, da)), left(nullptr), right(nullptr) {}
  };

private:
  std::shared_ptr<Node> root;
  size_t size;
  void merge(std::shared_ptr<Node> &a, std::shared_ptr<Node> &b) {

    if (b == nullptr) {
      return;
    }
    if (a == nullptr) {
      a = b;
      return;
    }
    if (*(a->data.second) > *(b->data.second)) {
      std::swap(a, b);
    }

    merge(a->right, b);
    if (a->left == nullptr) {
      a->left = a->right;
      a->right = nullptr;
      a->data.first = 1;
    } else {
      if (a->left->data.first < a->right->data.first) {
        std::swap(a->left, a->right);
      }
      a->data.first = a->right->data.first + 1;
    }
  }

public:
  void meld(minHBLT<T> &other) {
    merge(root, other.root);
    size += other.size;
    other.size = 0;
    other.root = nullptr;
  }

  void push(const T &element) {
    std::shared_ptr<Node> new_node = std::make_shared<Node>(Node(element));
    merge(root, new_node);
    size++;
  }
  void push(const std::shared_ptr<T> element) {
    std::shared_ptr<Node> new_node = std::make_shared<Node>(Node(element));
    merge(root, new_node);
    size++;
  }

  std::shared_ptr<T> top() {
    if (root == nullptr) {
      return nullptr;
    }
    return root->data.second;
  }

  void pop() {
    if (root == nullptr) {
      throw std::out_of_range("pop from empty heap");
    }
    std::shared_ptr<Node> left = root->left;
    std::shared_ptr<Node> right = root->right;
    merge(left, right);
    root = left;
    size--;
  }

  size_t get_size() { return size; }

  bool empty() { return size == 0; }

  minHBLT() : root(nullptr), size(0) {}
  minHBLT(std::vector<T> &vec) : root(nullptr), size(0) {
    for (auto &i : vec) {
      push(i);
      size++;
    }
  }
};
