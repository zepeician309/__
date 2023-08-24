#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

const int BUFFER_SIZE = 5;
const int END_TIME = 20;

std::queue<int> worker1Buffer;
std::queue<int> worker2Buffer;
std::queue<int> worker3Buffer;
std::mutex mtx;
std::condition_variable worker1CV, worker2CV, worker3CV, worker4CV;
std::vector<int> data_worker1, data_worker2, data_worker3, result_worker4;

void worker1() {
    for (int time = 0; time < END_TIME; ++time) {
        std::unique_lock<std::mutex> lock(mtx);
        // Wait if worker1Buffer is full
        worker1CV.wait(lock, [] { return worker1Buffer.size() < BUFFER_SIZE; });

        worker1Buffer.push(time);
        data_worker1.push_back(time);

        lock.unlock();
        worker2CV.notify_one();
    }
}

void worker2() {
    for (int time = 0; time < END_TIME; ++time) {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait if both worker1Buffer is empty and worker2Buffer is full
        worker2CV.wait(lock, [&] {
            return !worker1Buffer.empty() && worker2Buffer.size() < BUFFER_SIZE;
        });

        int time_stamp_to_pass = worker1Buffer.front();
        worker1Buffer.pop();
       
        data_worker2.push_back(time_stamp_to_pass);

        worker2Buffer.push(time_stamp_to_pass);
        lock.unlock();
        worker3CV.notify_one();
        worker1CV.notify_one();
    }
}

void worker3() {
    for (int time = 0; time < END_TIME; ++time) {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait if both worker2Buffer is empty and worker3Buffer is full
        worker3CV.wait(lock, [&] {
            return !worker2Buffer.empty() && worker3Buffer.size() < BUFFER_SIZE;
        });

        int time_stamp_to_pass = worker2Buffer.front();
        worker2Buffer.pop();
       
        data_worker3.push_back(time_stamp_to_pass);

        worker3Buffer.push(time_stamp_to_pass);
        lock.unlock();
        worker4CV.notify_one();
        worker2CV.notify_one();
    }
}

void worker4() {
    for (int time = 0; time < END_TIME; ++time) {
        std::unique_lock<std::mutex> lock(mtx);
        // Wait if worker3Buffer is empty
        worker4CV.wait(lock, [] { return !worker3Buffer.empty(); });

        int time_stamp_to_process = worker3Buffer.front();
        worker3Buffer.pop();
        result_worker4.push_back(time_stamp_to_process);

        lock.unlock();
        worker3CV.notify_one();
    }
}

int main() {
    std::thread worker1Thread(worker1);
    std::thread worker2Thread(worker2);
    std::thread worker3Thread(worker3);
    std::thread worker4Thread(worker4);

    worker1Thread.join();
    worker2Thread.join();
    worker3Thread.join();
    worker4Thread.join();

    for (const auto &ele : data_worker1) std::cout << ele << " ";
    std::cout << std::endl;

    for (const auto &ele : data_worker2) std::cout << ele << " ";
    std::cout << std::endl;

    for (const auto &ele : data_worker3) std::cout << ele << " ";
    std::cout << std::endl;

    for (const auto &ele : result_worker4) std::cout << ele << " ";
    std::cout << std::endl;

    return 0;
}
