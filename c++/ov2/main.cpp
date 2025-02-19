#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <queue>
#include <condition_variable>

class Workers {
public:
    explicit Workers(unsigned int num_threads): num_threads(num_threads){};

    void start(){
        thread_join = false;
        threads.erase(threads.begin(), threads.end());

        for(size_t i = 0; i < num_threads; i++) {
            threads.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;

                    std::unique_lock cv_lock(start_thread_mutex);
                    if(tasks.empty()){
                        start_thread.wait(cv_lock, [this]{ return !tasks.empty() || thread_join;});
                    }

                    // if a join is called, join here
                    if(thread_join && tasks.empty()){
                        return;
                    }

                    // only lock while fetching new task
                    {
                        std::lock_guard<std::mutex> guard(tasks_mutex);
                        task = std::move(tasks.front());
                        tasks.pop();
                        cv_lock.unlock();

                        if (!tasks.empty()) {
                            start_thread.notify_one();
                        }
                    }
                    task();
                }
            });
        }

        for(unsigned int i = 0; i < tasks.size() && i < threads.size(); i++){
            start_thread.notify_one();
        }
    }

    void join(){
        thread_join = true;
        start_thread.notify_all();
        for(auto &thread: threads){
            thread.join();
        }
    }


    void post(const std::function<void()> &task){
        if(thread_join){
            throw std::logic_error("Cannot add task while worker is joining");
        }

        {
            std::lock_guard<std::mutex> guard(tasks_mutex);
            tasks.emplace(task);
        }
        start_thread.notify_one();
    }

    void post_timeout(const std::function<void()> &task, int time){
        if(thread_join){
            throw std::logic_error("Cannot add task while worker is joining");
        };

        threads.emplace_back([task, time] {
            std::this_thread::sleep_for(std::chrono::milliseconds(time));
            task();
        });
    }

private:
    const unsigned int num_threads;
    std::vector<std::thread> threads;
    std::mutex tasks_mutex;
    std::queue<std::function<void()>> tasks;
    std::atomic<bool> thread_join{false};
    std::condition_variable start_thread;
    std::mutex start_thread_mutex;
};

int main() {
    Workers worker_threads(4);
    Workers event_loop(1);

    worker_threads.start(); // Create 4 internal threads
    event_loop.start(); // Create 1 internal thread

    std::cout << "Started" << std::endl;
    worker_threads.post([](){
        std::cout << "hello from A" << std::endl;
    });
    worker_threads.post ([](){
        std::cout << "hello from B" << std::endl;
    });
    event_loop.post ([](){
        std::cout << "hello from C" << std::endl;
    });
    event_loop.post([](){
        std::cout << "hello from D" << std::endl;
    });

    event_loop.post_timeout([]{
        std::cout << "hello from E" << std::endl;
    }, 2000);

    event_loop.post_timeout([]{
        std::cout << "hello from F" << std::endl;
    }, 1000);

    worker_threads.join(); // Calls join () on the worker threads
    event_loop.join(); // Calls join () on the event thread

    return 0;
}
