#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool(size_t num_threads) : is_shutdown(false) {
        initialize(num_threads);
    }

    void initialize(size_t num_threads);
    void run_one_job(std::function<void(size_t)> func, size_t index);
    void join();
    ~ThreadPool();

private:
    void threadWorker();

private:
    std::vector<std::thread> threads;
    std::queue<std::pair<std::function<void(size_t)>, size_t>> job_queue;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool is_shutdown;
};

void ThreadPool::initialize(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([this] { threadWorker(); });
    }
}

void ThreadPool::run_one_job(std::function<void(size_t)> func, size_t index) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        job_queue.push(std::make_pair(func, index));
    }
    condition.notify_one();
}

void ThreadPool::join() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        is_shutdown = true;
    }
    condition.notify_all();

    for (auto& thread : threads) {
        thread.join();
    }
}

ThreadPool::~ThreadPool() {
    if (!is_shutdown) {
        join();
    }
}

void ThreadPool::threadWorker() {
    while (true) {
        std::pair<std::function<void(size_t)>, size_t> job;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return is_shutdown || !job_queue.empty(); });

            if (is_shutdown && job_queue.empty()) {
                return;
            }

            job = job_queue.front();
            job_queue.pop();
        }

        // Call the provided function with the specified index
        job.first(job.second);
    }
}

int main() {
    ThreadPool pool(4); // Create a thread pool with 4 worker threads

    for (size_t i = 0; i < 10; ++i) {
        pool.run_one_job([](size_t index) {
            std::cout << "Job " << index << " executed by thread " << std::this_thread::get_id() << std::endl;
        }, i);
    }

    pool.join(); // Wait for all jobs to complete

    return 0;
}
