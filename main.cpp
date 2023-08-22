#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool() : is_shutdown(false) {}

    void initialize(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([this] { threadWorker(); });
        }
    }

    void run_one_job(std::function<void()> func) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            job_queue.push(func);
        }
        condition.notify_one();
    }

    void join() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            is_shutdown = true;
        }
        condition.notify_all();

        for (auto& thread : threads) {
            thread.join();
        }
    }

    ~ThreadPool() {
        if (!is_shutdown) {
            join();
        }
    }

private:
    void threadWorker() {
        while (true) {
            std::function<void()> func;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                condition.wait(lock, [this] { return is_shutdown || !job_queue.empty(); });

                if (is_shutdown && job_queue.empty()) {
                    return;
                }

                func = job_queue.front();
                job_queue.pop();
            }

            func();
        }
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> job_queue;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool is_shutdown;
};

int main() {
    ThreadPool pool;
    pool.initialize(4); // Create a thread pool with 4 worker threads

    for (int i = 0; i < 10; ++i) {
        pool.run_one_job([i] {
            std::cout << "Job " << i << " executed by thread " << std::this_thread::get_id() << std::endl;
        });
    }

    pool.join(); // Wait for all jobs to complete

    return 0;
}
