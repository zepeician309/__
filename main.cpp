#include <iostream>
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>

class ThreadPool {
public:
    ThreadPool(int numThreads) : stop(false) {
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template <typename Func>
    void addTask(Func f) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::function<void()>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

    void join() {
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// Placeholder functions
void compute_basevar(int t) {
    // Placeholder implementation for computing base variables
}

void compute_derivedvar(int t, int i) {
    // Placeholder implementation for computing derived variables
}

void compute_regression(int t) {
    // Placeholder implementation for computing regression
}

int main() {
    const int numThreads = 4;
    ThreadPool threadPool(numThreads);

    std::vector<int> allTimestamps; // Replace with your actual timestamps

    int last_compute_basevar_t = -1;
    int last_compute_derivedvar_t = -1;

    for (const int& t : allTimestamps) {
        std::mutex basevarMutex;
        std::condition_variable basevarCV;

        int derivedvar_finished_count = 0;
        std::mutex derivedvarMutex;
        std::condition_variable derivedvarCV;

        // Compute Base Variables
        threadPool.addTask([t, &basevarMutex, &basevarCV, &last_compute_basevar_t] {
            compute_basevar(t);
            {
                std::lock_guard<std::mutex> lock(basevarMutex);
                last_compute_basevar_t = t;
            }
            basevarCV.notify_one();
        });

        // Compute Derived Variables in Parallel
        for (int i = 0; i < numThreads; ++i) {
            threadPool.addTask([t, i, &basevarMutex, &basevarCV, &last_compute_basevar_t, &last_compute_derivedvar_t, &derivedvar_finished_count, &derivedvarMutex, &derivedvarCV] {
                {
                    std::unique_lock<std::mutex> lock(basevarMutex);
                    basevarCV.wait(lock, [&last_compute_basevar_t, t] { return last_compute_basevar_t >= t; });
                }

                compute_derivedvar(t, i);

                {
                    std::lock_guard<std::mutex> lock(derivedvarMutex);
                    last_compute_derivedvar_t = t;
                }

                // Increment the counter and notify
                {
                    std::lock_guard<std::mutex> lock(derivedvarMutex);
                    derivedvar_finished_count++;
                    if (derivedvar_finished_count == numThreads) {
                        derivedvarCV.notify_one();
                    }
                }
            });
        }

        // Perform Regression Computation
        threadPool.addTask([t, &basevarMutex, &basevarCV, &derivedvarMutex, &derivedvarCV, &derivedvar_finished_count] {
            {
                std::unique_lock<std::mutex> lock(basevarMutex);
                basevarCV.wait(lock, [&last_compute_derivedvar_t, t] { return last_compute_derivedvar_t >= t; });
            }

            // Wait for all compute_derivedvar computations to complete
            {
                std::unique_lock<std::mutex> lock(derivedvarMutex);
                derivedvarCV.wait(lock, [numThreads, &derivedvar_finished_count] { return derivedvar_finished_count == numThreads; });
            }

            compute_regression(t);
        });

        // If you have more computations, add them here using threadPool.addTask
    }

    // Wait for all tasks to complete
    threadPool.join();

    return 0;
}
