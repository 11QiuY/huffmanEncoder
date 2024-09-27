#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#include "threadsafe_queue.hpp"
#include <future>
#include <iostream>
#include <thread>
#include <type_traits>

// 封装一个函数对象，可以用来执行函数，其可以返回任意值
class function_wrapper {
private:
  struct impl_base {
    virtual void call() = 0;
    virtual ~impl_base() {}
  };
  std::unique_ptr<impl_base> impl;
  template <typename F> struct impl_type : impl_base {
    F f;
    impl_type(F &&f_) : f(std::move(f_)) {}
    void call() override { f(); }
  };

public:
  template <typename F>
  function_wrapper(F &&f) : impl(new impl_type<F>(std::move(f))) {}
  void operator()() { impl->call(); }
  function_wrapper() = default;
  function_wrapper(function_wrapper &&other) : impl(std::move(other.impl)) {}
  function_wrapper &operator=(function_wrapper &&other) {
    impl = std::move(other.impl);
    return *this;
  }
  function_wrapper(const function_wrapper &) = delete;
  function_wrapper(function_wrapper &) = delete;
  function_wrapper &operator=(const function_wrapper &) = delete;
};

class ThreadPoolwithReturn {
private:
  using Task = function_wrapper;
  ThreadSafeQueue<function_wrapper> task_queue;
  int num_threads;
  std::atomic<bool> running;
  std::vector<std::thread> workers;
  std::mutex cv_m;
  std::condition_variable cv;

  void worker_thread() {
    while (running) {
      Task task;
      if (task_queue.try_pop(task)) {
        try {
          task();
        } catch (const std::exception &e) {
          std::cerr << "exception caught in ThreadPool: " << e.what()
                    << std::endl;
        }
      } else {
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk, [this] { return !running || !task_queue.empty(); });
      }
    }
  }

public:
  template <typename F> auto submit(F task) {
    using result_type = std::result_of_t<F()>;
    std::packaged_task<result_type()> ptask(std::move(task));
    auto future = ptask.get_future();
    task_queue.push(std::move(ptask));
    cv.notify_one();
    return future;
  }

  ThreadPoolwithReturn(const int &num_threads)
      : num_threads(num_threads), task_queue(100), running(true) {
    for (int i = 0; i < num_threads; ++i) {
      workers.emplace_back([this] { worker_thread(); });
    }
  }
  ThreadPoolwithReturn() : task_queue(100), running(true) {
    num_threads = std::thread::hardware_concurrency();
    for (int i = 0; i < num_threads; ++i) {
      workers.emplace_back([this] { worker_thread(); });
    }
  }
  ~ThreadPoolwithReturn() {
    running = false;
    cv.notify_all();
    for (auto &worker : workers) {
      if (worker.joinable())
        worker.join();
    }
  }
};

#endif