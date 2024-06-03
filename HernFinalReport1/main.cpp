//2-1 : Dynamic Queueing 미완성
//2-2 : Alarm Clock 부분 완성(shell 완성, monitor 미완성)
//2-3 : CLI 완성(&처리, commands(echo, dummy, gcd, prime, sum), option(-n, -d, -p, -m))
// 전체적으로 Dynamic Queueing 처리 대신에 주신 예제 1의 mutext를 사용하여 구현하였습니다. 
// 최종 수정완료

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <atomic>
#include <windows.h>

using namespace std;
atomic<int> pid(0); // 프로세스 id


// 프로세스 옵션정보 
struct OPTION {
    string cmd;
    string message;   //  출력 메시지, e.g echo
    int process_type; // 0: BG(background process),  1: FG(foreground process)
    int x, y;   // 명령의 인수
    int n;  // 같은 명령을 실행하는 프로세스 x 개 생성
    int d;  // 실행 시간이 x 를 넘으면 프로세스 종료 (duration).
    int p;  // 같은 작업을 x 초마다 반복 (period)
    int m;  // 데이터를 x 개로 분할해서 병렬 처리 (multithread) : (sum 명령에만 사용)
};

// 명령어 옵션 해석
OPTION parse_option(const vector<string>& args) {
    OPTION option = { "", "", 1, 0, 0, 1, 1, 1, 0 }; // 디폴트 
    
    //BG, FG 체크
    option.process_type = (args[0][0] == '&') ?  0: 1;
    if(option.process_type == 0) // &
        option.cmd = args[0].substr(1, args[0].length());
    else
        option.cmd = args[0];

    for (size_t i = 1; i < args.size(); ++i) {
        if (option.cmd == "echo" && i == 1) {
            option.message = args[i];
        }
        else if ((option.cmd == "sum" || option.cmd == "prime") && i == 1) {
            option.x = stoi(args[i]);
        }
        else if (option.cmd == "gcd" && i == 1) {
            option.x = stoi(args[i]);
            if (i + 1 < args.size()) {
                option.y = stoi(args[i + 1]);
                ++i;
            }
        }
        // -n, -d, -p, -m 처리 
        else if (args[i] == "-n" && i + 1 < args.size()) {
            option.n = stoi(args[i + 1]);
            ++i;
        }
        else if (args[i] == "-d" && i + 1 < args.size()) {
            option.d = stoi(args[i + 1]);
            ++i;
        }
        else if (args[i] == "-p" && i + 1 < args.size()) {
            option.p = stoi(args[i + 1]);
            ++i;
        }
        else if (args[i] == "-m" && i + 1 < args.size()) {
            option.m = stoi(args[i + 1]);
            ++i;
        }
    }
    return option;
}

// echo 명령 실행
void executeEcho(const string& message, int pid, int n, int process_type) {
    cout << pid << ((process_type == 0) ?  "B": "F") << "(" << n << "): " << message << endl;
}

// sum 명령 실행
void executeSum(int x, int numThreads, int pid, int n, int process_type) {
    int sum = 0;
    for (int i = 1; i <= numThreads; ++i) {
        sum += x * i;
    }
    cout << pid << ((process_type == 0) ? "B" : "F") << "(" << n << "): " << " Sum: " << sum << endl;
}

// gcd 명령 실행
void executeGCD(int x, int y, int pid, int n, int process_type) {
    while (y != 0) {
        int temp = y;
        y = x % y;
        x = temp;
    }
    cout << pid << ((process_type == 0) ? "B" : "F") <<"(" << n << "): " << ": GCD: " << x << endl;
}

// prime 명령 실행
void executePrime(int x, int pid, int n, int process_type) {
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
    cout << pid << ((process_type == 0) ? "B" : "F") << "(" << n << "): " << "prime count(" << x << "):" << countPrimes(x) << endl;
}

// dummy 명령 실행
void executeDummy(int pid, int n, int process_type) {
    cout << pid << ((process_type == 0) ? "B" : "F") << "(" << n << "): " << ": Dummy" << endl;
}

// 명령과 옵션을 분석하여 실행
void exec(const vector<string>& args) {
    static mutex mtx;
    OPTION option = parse_option(args); // 명령 옵션 분석
    int current_pid = ++pid;  // 프로세스 id 

    string cmd = option.cmd;  // 명령어 
    int process_type = option.process_type; // BG, FG 프로세스 구분

    if (cmd == "echo") { // echo 명령 처리
        string message = option.message;
        int period = option.p;
        int duration = option.d;
        int numProcess = option.n;
        

        if (!message.empty()) {
            for (int n = 0; n < numProcess; ++n) {
                thread([message, period, duration, n, current_pid, process_type]() {
                    int timeElapsed = 0;
                    int count = 0;

                    while (timeElapsed <= duration) {
                        if (count == period) {
                            //lock_guard<mutex> lock(mtx);
                            mtx.lock(); // 뮤텍스로 스레드 순서화
                            executeEcho(message, current_pid, n, process_type);
                            mtx.unlock();
                            count = 0;
                        }
                        this_thread::sleep_for(chrono::seconds(1));
                        timeElapsed++;
                        count++;
                    }
                    }).detach();
            }
            if (duration > 0) {
                //this_thread::sleep_for(chrono::seconds(duration));
                Sleep(duration * 1000);

            }
        }
    }
    else if (cmd == "sum") { // sum 명령 처리
        int x = option.x;
        int numThreads = option.m;
        int numProcess = option.n;

        for (int n = 0; n < numProcess; ++n) {
            thread([x, numThreads, current_pid, n, process_type]() {
                mtx.lock();
                executeSum(x, numThreads, current_pid, n, process_type);
                mtx.unlock();
                }).detach();
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
    else if (cmd == "gcd") { // gcd 명령 처리
        int x = option.x;
        int y = option.y;
        int numProcess = option.n;

        for (int n = 0; n < numProcess; ++n) {
            thread([x, y, current_pid, n, process_type]() {
                mtx.lock();
                executeGCD(x, y, current_pid, n, process_type);
                mtx.unlock();
                }).detach();
        }
        //this_thread::sleep_for(chrono::seconds(1));
        Sleep(1000);
    }
    else if (cmd == "prime") { // prime 명령 처리
        int x = option.x;
        int numProcess = option.n;

        for (int n = 0; n < numProcess; ++n) {
            thread([x, current_pid, n, process_type]() {
                mtx.lock();
                executePrime(x, current_pid, n, process_type);
                mtx.unlock();
                }).detach();
        }
        this_thread::sleep_for(chrono::seconds(1));
    }
    else if (cmd == "dummy") { // dummy 명령 처리
        int numProcess = option.n;

        for (int n = 0; n < numProcess; ++n) {
            thread([current_pid, n, process_type]() {
                //lock_guard<mutex> lock(mtx);
                mtx.lock();
                executeDummy(current_pid, n, process_type);
                mtx.unlock();
                }).detach();
        }
        //this_thread::sleep_for(chrono::seconds(1));
        Sleep(1000);
    }
}

// line을 ';' 구분자로 분리하여 명령(commands) 생성
vector<string> split_commands(const string& line) {
    vector<string> commands;
    istringstream stream(line);
    string command;
    while (getline(stream, command, ';')) {
        commands.push_back(command);
    }
    return commands;
}

// 하나의 명령(command)을 공백으로 구분하여 args 생성
vector<string> parse_args(const string& command) {
    vector<string> args;
    istringstream stream(command);
    string arg;
    while (stream >> arg) {
        args.push_back(arg);
    }
    return args;
}

// 명령어 토큰을 분석하여 스레드(exec)로 실행
void execute_command(const string& command) {
    vector<string> args = parse_args(command); // 명령어 토큰 분석
    if (!args.empty()) {
        if (args[0][0] == '&') { //BG 프로세스
            thread(exec, args).detach();
        }
        else {
            exec(args);
        }
    }
}

// Shell 함수 구현: commands.txt에서 명령을 읽고 명령 실행
void shell(const string& filename)
{
    ifstream file(filename); // 파일 개방
    string line;
    while (getline(file, line))
    {
        cout << "prompt> " << line << endl; // 프롬프트 출력 

        vector<string> commands = split_commands(line); // 라인을 명령들로 분리
        for (const string& command : commands) // 각 명령을 처리 
        { 
            execute_command(command);
            if (command[0] != '&') {  // 백그라운드 프로세스이면, 포그라운드 대기
                //this_thread::sleep_for(chrono::seconds(1)); 
                Sleep(1000);

            }
        }
    }
    file.close();
}

int main()
{
    // 스레드로 shell processes 시작 
    thread shell_thread(shell, "commands.txt");
    //thread monitor_thread(monitor, ref(dq));

    // 스레드가 끝나길 대기
    shell_thread.join();
    //monitor_thread.join(); // 완성 안됨 

    return 0;
}
