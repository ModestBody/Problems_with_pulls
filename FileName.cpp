#include <vector>
#include <thread>
#include <future>
#include <iostream>
#include <random>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();
    template<typename F>
    auto enqueue(F&& f) -> std::future<void>;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
};

ThreadPool::ThreadPool(size_t num_threads) {
    for(size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                    if(this->stop && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(auto &worker : workers) worker.join();
}

template<typename F>
auto ThreadPool::enqueue(F&& f) -> std::future<void> {
    auto task = std::make_shared<std::packaged_task<void()>>(std::forward<F>(f));
    std::future<void> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace([task]{ (*task)(); });
    }
    condition.notify_one();
    return res;
}

ThreadPool pool(std::thread::hardware_concurrency());

void quicksort(int *array, long left, long right, std::shared_ptr<std::promise<void>> parent_promise = nullptr) {
    if(left >= right) {
        if (parent_promise) parent_promise->set_value();
        return;
    }

    long left_bound = left, right_bound = right;
    long middle = array[(left + right) / 2];

    do {
        while(array[left_bound] < middle) left_bound++;
        while(array[right_bound] > middle) right_bound--;
        if (left_bound <= right_bound) std::swap(array[left_bound++], array[right_bound--]);
    } while (left_bound <= right_bound);

    auto left_promise = std::make_shared<std::promise<void>>();
    auto right_promise = std::make_shared<std::promise<void>>();

    if (right_bound - left > 100000) {
        pool.enqueue([=]{ quicksort(array, left, right_bound, left_promise); });
    } else {
        quicksort(array, left, right_bound, left_promise);
    }

    if (right - left_bound > 100000) {
        pool.enqueue([=]{ quicksort(array, left_bound, right, right_promise); });
    } else {
        quicksort(array, left_bound, right, right_promise);
    }

    std::async(std::launch::async, [left_promise, right_promise, parent_promise]() {
        left_promise->get_future().wait();
        right_promise->get_future().wait();
        if (parent_promise) parent_promise->set_value();
    });
}
