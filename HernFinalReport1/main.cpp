#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <cmath>
#include <cstring>
#include <windows.h>

using namespace std;

bool running = true;
int simulated_time = 0;
mutex mtx;
bool dqMonitorRequested = false; // 전역 변수로 선언

class DynamicQueue {
public:
    DynamicQueue(size_t maxSize) : maxSize(maxSize) {}

    void enqueue(const string& command) {
        unique_lock<mutex> lock(mtx);
        while (queue.size() >= maxSize) {
            cv.wait(lock);
        }
        queue.push(command);
        cv.notify_all();
    }

    string dequeue() {
        unique_lock<mutex> lock(mtx);
        while (queue.empty()) {
            cv.wait(lock);
        }
        string command = queue.front();
        queue.pop();
        cv.notify_all();
        return command;
    }

    size_t size() {
        unique_lock<mutex> lock(mtx);
        return queue.size();
    }

private:
    size_t maxSize;
    queue<string> queue;
    mutex mtx;
    condition_variable cv;
};

void executeEcho(const string& str) {
    cout << str << endl;
}

void executeDummy() {
    // Dummy process does nothing
}

void executeGCD(int x, int y) {
    auto gcd = [](int a, int b) {
        while (b != 0) {
            int t = b;
            b = a % b;
            a = t;
        }
        return a;
        };
    cout << "GCD of " << x << " and " << y << " is " << gcd(x, y) << endl;
}

void executePrime(int x) {
    auto countPrimes = [](int x) {
        vector<bool> is_prime(x + 1, true);
        is_prime[0] = is_prime[1] = false;
        for (int i = 2; i <= sqrt(x); ++i) {
            if (is_prime[i]) {
                for (int j = i * i; j <= x; j += i) {
                    is_prime[j] = false;
                }
            }
        }
        return count(is_prime.begin(), is_prime.end(), true);
        };
    cout << "Number of primes up to " << x << " is " << countPrimes(x) << endl;
}

void executeSum(int x, int numThreads) {
    auto sum = [](int start, int end) {
        long long result = 0;
        for (int i = start; i <= end; ++i) {
            result = (result + i) % 1000000;
        }
        return result;
        };

    vector<thread> threads;
    vector<long long> results(numThreads);
    int chunkSize = x / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int start = i * chunkSize + 1;
        int end = (i == numThreads - 1) ? x : (i + 1) * chunkSize;
        threads.emplace_back([i, start, end, &results, sum]() {
            results[i] = sum(start, end);
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    long long totalSum = 0;
    for (const auto& res : results) {
        totalSum = (totalSum + res) % 1000000;
    }
    cout << "Sum " << x << " % 1,000,000 :" << totalSum << endl;
}

char** parse(const char* command) {
    vector<string> tokens;
    stringstream ss(command);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }

    char** args = new char* [tokens.size() + 1];
    for (size_t i = 0; i < tokens.size(); ++i) {
        args[i] = new char[tokens[i].size() + 1];
        strcpy_s(args[i], tokens[i].size() + 1, tokens[i].c_str());
    }
    args[tokens.size()] = new char[1];
    args[tokens.size()][0] = '\0';  // Null-terminate the array

    return args;
}
//
typedef struct {
    string  cmd;
    string  message;  // for echo
    int x, y;
    int n;
    int d;
    int p;
    int m;
} OPTION;

OPTION parse_option(char** args)
{
    OPTION  option = {"echo", "abc", 1, 50, 5, 4}; 
    option.cmd = args[0];

    for (int i = 1; strlen(args[i]) != 0; ++i) 
    {
            if (option.cmd == "echo" && i == 1) {
                option.message = args[i];
            }
            else if (option.cmd == "sum" && i == 1) {
                option.x = stoi(args[i]);
            }
            else if (option.cmd == "prime" && i == 1) {
                option.x = stoi(args[i]);
            }
            else if (option.cmd == "gcd" && i == 1) {
                option.x = stoi(args[i]);
                option.y = stoi(args[i+1]);
                i++;
            }
            // -n, -d, -p, -m 처리 
            else if (string(args[i]) == "-n" && strlen(args[i + 1]) != 0) {
                option.n = stoi(args[i + 1]);
                ++i;
            }
            else if (string(args[i]) == "-d" && strlen(args[i + 1]) != 0) {
                option.d = stoi(args[i + 1]);
                ++i;
            }
            else if (string(args[i]) == "-p" && strlen(args[i + 1]) != 0) {
                option.p = stoi(args[i + 1]);
                ++i;
            }
            else if (string(args[i]) == "-m" && strlen(args[i + 1]) != 0) { //sum 명령에만 사용
                option.m = stoi(args[i + 1]);
                ++i;
            }
        }
    return option;
}

void exec(char** args, DynamicQueue& dq) {

    OPTION option = parse_option(args);


    string cmd = args[0];

    if (cmd == "echo") {
        string message;
        int period = 0;
        int duration = 0;
        int numProcess = 1; // 실제는 스레드

        for (int i = 1; strlen(args[i]) != 0; ++i)
        {
            if (string(args[i]) == "-p" && args[i + 1] != nullptr) {
                period = stoi(args[i + 1]);
                ++i;
            }
            else if (string(args[i]) == "-d" && args[i + 1] != nullptr) {
                duration = stoi(args[i + 1]);
                ++i;
            }
            else if (string(args[i]) == "-n" && args[i + 1] != nullptr) {
                numProcess = stoi(args[i + 1]);
                ++i;
            }
            else {
                message = args[i];
            }
        }

        // Execute echo command with specified parameters
        if (!message.empty()) {
            for (int i = 0; i < numProcess; ++i) {
                thread([message, period, duration, i]() {  // 프로세스 대신 스레드로 구현
                    int timeElapsed = 0;
                    int count = 0;
                    while (running && (duration == 0 || timeElapsed < duration)) {
                        if (count == period) { // period마다 출력 
                            //lock_guard<mutex> lock(mtx);
                            mtx.lock();
                            cout << "Process(Thread) " << i << ": ";
                            executeEcho(message);
                            mtx.unlock();
                            count = 0;
                        }

                        //this_thread::sleep_for(chrono::seconds(1));
                        //Sleep(1000);
                        timeElapsed++;
                        count++;
                    }
                    }).detach();
            }

            if (duration > 0) {
                //this_thread::sleep_for(chrono::seconds(duration));
                Sleep(duration*1000);  // 프로세스 종료를 위해 대기시간이 필요
                running = false; // Set running to false to stop threads
            }
        }
        /*
        if (!message.empty()) {
            vector<thread> threads;
            for (int i = 0; i < numProcess; ++i) {
                threads.emplace_back([message, period, duration, i]() {
                    int timeElapsed = 0;
                    int count = 0;
                    while (running && (duration == 0 || timeElapsed <= duration)) {
                        if (count == period) { // period마다 출력 
                            mtx.lock();
                             cout << "Process(Thread) " << i << ": ";
                            executeEcho(message);
                            mtx.unlock();
                            count = 0;
                        }
                        //this_thread::sleep_for(chrono::seconds(1));
                        timeElapsed++;
                        count++;
                    }
                    });
            }
            if (duration > 0) {
                //this_thread::sleep_for(chrono::seconds(duration));
                Sleep(100);
            }
            // Set running to false to stop threads
            running = false;

            // Wait for threads to finish
            for (auto& t : threads) {
                t.join();
            }
        }
        */
    }
    else if (cmd == "dummy") {
        executeDummy();
    }
    else if (cmd == "gcd") {
        if (args[1] != nullptr && args[2] != nullptr) {
            int x = stoi(args[1]);
            int y = stoi(args[2]);


            executeGCD(x, y);
        }
    }
    else if (cmd == "prime") {
        if (args[1] != nullptr) {
            int x = stoi(args[1]);
            executePrime(x);
        }
    }
    else if (cmd == "sum") {
        cout << "sum******" << endl;

        if (args[1] != nullptr) {
            int x = stoi(args[1]);
            int numThreads = 1;
            int numProcess = 1;
            for (int i = 2; strlen(args[i]) != 0; ++i) {  
                cout << "i=" << i<< endl;
                if (string(args[i]) == "-m" && args[i + 1] != nullptr) {
                    numThreads = stoi(args[i + 1]);
                    cout << "numThreads=" << numThreads << endl;
                    ++i;
                }
                else if (string(args[i]) == "-n" && args[i + 1] != nullptr) {
                    numProcess = stoi(args[i + 1]);
                    cout << "numProcess= " << numProcess << endl;
                    ++i;
                }

            }
            // cout << numThreads  << endl;
            // cout << numProcess << endl;
            for (int i = 0; i < numProcess; ++i) {
                thread([x, numThreads, i]() {  // 프로세스 대신 스레드로 구현
                    mtx.lock();
                    cout << "Process(Thread) " << i << ": ";
                    executeSum(x, numThreads);
                    mtx.unlock();
                      }).detach();
            }
            Sleep(1000);  // 프로세스 종료를 위해 대기시간이 필요
            running = false;
        }
    }
}

void shell(const string& filename, DynamicQueue& dq) {
    ifstream file(filename);
    string line;
    while (running && getline(file, line)) {
        char** args = parse(line.c_str());
        thread(exec, args, ref(dq)).detach();  // Create a detached thread to execute the command
        {
            lock_guard<mutex> lock(mtx);
            simulated_time += 1;  // Increment simulated time by 1 second
        }

        if (dqMonitorRequested) {
            cout << "Dynamic Queue size: " << dq.size() << endl;
            dqMonitorRequested = false; // 큐 크기 확인 후 변수 초기화
        }
    }
    file.close();
}

void monitor(DynamicQueue& dq)
{
    int last_checked_time = 0;

    while (running) {
        int current_time;
        {
            lock_guard<mutex> lock(mtx);
            current_time = simulated_time;
        }

        if (current_time - last_checked_time >= 5) {
            dqMonitorRequested = true; // 5
            cout << "Dynamic Queue size: " << dq.size() << endl;
            last_checked_time = current_time;
        }

        this_thread::sleep_for(chrono::milliseconds(1000));  // Small sleep to prevent busy waiting
    }
}

int main()
{
    DynamicQueue dq(5);

    // Start shell and monitor processes
    thread shell_thread(shell, "commands.txt", ref(dq));
    thread monitor_thread(monitor, ref(dq));

    // Wait for threads to finish
    shell_thread.join();
    monitor_thread.join();

    return 0;
}
