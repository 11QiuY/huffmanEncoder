#ifndef THREADSAFE_QUEUE_HPP
#define THREADSAFE_QUEUE_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

template <typename T> class ThreadSafeQueue {
private:
  mutable std::mutex mut;
  int max_length;
  std::queue<std::shared_ptr<T>> data_queue;
  std::condition_variable data_cond;
  std::condition_variable data_cond_full;

public:
  ThreadSafeQueue(int max_length = 100) : max_length(max_length) {}
  void push(T new_value) {
    std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
    if (data_queue.size() >= max_length) {
      std::unique_lock<std::mutex> lk(mut);
      data_cond_full.wait(lk,
                          [this] { return data_queue.size() < max_length; });
    }
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(data);
    data_cond.notify_one();
  }

  void wait_and_pop(T &value) {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, [this] { return !data_queue.empty(); });
    value = std::move(*data_queue.front());
    data_queue.pop();
    data_cond_full.notify_one();
  }

  std::shared_ptr<T> wait_and_pop() {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, [this] { return !data_queue.empty(); });
    std::shared_ptr<T> res = data_queue.front();
    data_queue.pop();
    data_cond_full.notify_one();
  }

  bool try_pop(T &value) {
    std::lock_guard<std::mutex> lk(mut);
    if (data_queue.empty())
      return false;
    value = std::move(*data_queue.front());
    data_queue.pop();
    data_cond_full.notify_one();
    return true;
  }

  std::shared_ptr<T> try_pop() {
    std::lock_guard<std::mutex> lk(mut);
    if (data_queue.empty())
      return std::shared_ptr<T>();
    std::shared_ptr<T> res = data_queue.front();
    data_queue.pop();
    data_cond_full.notify_one();
    return res;
  }

  bool empty() const {
    std::lock_guard<std::mutex> lk(mut);
    return data_queue.empty();
  }
};

#endif