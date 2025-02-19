#include <iostream>
#include <thread>
#include <vector>

int main() {

    int start_number;
    std::cout << "What is the start number? ";
    std::cin >> start_number;

    int end_number;
    std::cout << "What is the end number? ";
    std::cin >> end_number;


    if(end_number <= start_number){
        std::cout << "End number must be higher than the start number" << std::endl;
        return 0;
    }

    // we END at end number
    end_number++;

    int num_threads;
    std::cout << "How many threads? ";
    std::cin >> num_threads;

    // we need at least one thread
    if(num_threads < 1){
        num_threads = 1;
    }

    std::vector<bool> results(end_number - start_number);

    for(auto val: results){
        val = false;
    };

    const auto is_prime = [](int number)->bool{
        if(number <= 1){
            return false;
        }

        for(int i = 2; i <= number - 1; i++){
            if(number % i == 0){
                return false;
            }
        }

        return true;
    };

    std::cout << "Checking primes from " << start_number << " to " << end_number - 1 << " with " << num_threads << " threads" << std::endl;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for(int thread_num = 0; thread_num < num_threads; thread_num++){
        threads.emplace_back([thread_num, &num_threads, &results, &start_number, &end_number, &is_prime](){
            for(int i = start_number + thread_num; i < end_number; i += num_threads){
                if(is_prime(i)){
                    results[i - start_number] = true;
                }
            }
        });
    }

    for(auto &thread: threads){
        thread.join();
    }

    for(size_t i = start_number; i < end_number; i++){
        if(results[i]){
            std::cout << i << " is a prime number" << std::endl;
        }
    }

    return 0;
}
