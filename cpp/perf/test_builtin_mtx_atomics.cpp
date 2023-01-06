#include <iostream> 
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <sstream>

#define NOF_LOOPS 100000000

using namespace std;
using namespace chrono;

class cea_timer {
public:
    cea_timer() = default;
    void start();
    double elapsed();
    string elapsed_in_string(int precision=9);
private:
    time_point<high_resolution_clock> begin;
    time_point<high_resolution_clock> end;
};

void pin_thread(int id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// shared variable
uint64_t plain_shared_cntr;
atomic<uint64_t> atm_shared_ctr;

// performance timer
cea_timer timer;

// mutex
mutex mtx;

void atomic_worker(int cpuid) {
    pin_thread(cpuid);
    uint64_t loops;

    timer.start();
    for (loops=0; loops<NOF_LOOPS; loops++) {
        atm_shared_ctr++;
    }
    cout << "atomic worker: " << timer.elapsed_in_string() << endl;
}

void builtin_worker(int cpuid) {
    pin_thread(cpuid);
    uint64_t loops;

    timer.start();
    for (loops=0; loops<NOF_LOOPS; loops++) {
        __sync_fetch_and_add(&plain_shared_cntr, 1);
    }
    cout << "builtin worker: " << timer.elapsed_in_string() << endl;
}

void mutex_worker(int cpuid) {
    pin_thread(cpuid);
    uint64_t loops;

    timer.start();
    for (loops=0; loops<NOF_LOOPS; loops++) {
        mtx.lock();
        plain_shared_cntr++;
        mtx.unlock();
    }
    cout << "mutex worker: " << timer.elapsed_in_string() << endl;
}

int main() {
    plain_shared_cntr = 0;
    thread builtin_thread1, builtin_thread2;
    builtin_thread1 = thread(&builtin_worker, 1);
    builtin_thread2 = thread(&builtin_worker, 2);
    builtin_thread1.join();
    builtin_thread2.join();
    cout << "Builtin Counter Value: " << plain_shared_cntr << endl << endl;

    atm_shared_ctr = 0;
    thread atomic_thread1, atomic_thread2;
    atomic_thread1 = thread(&atomic_worker, 1);
    atomic_thread2 = thread(&atomic_worker, 2);
    atomic_thread1.join();
    atomic_thread2.join();
    cout << "Atomic Counter Value: " << plain_shared_cntr << endl << endl;

    plain_shared_cntr = 0;
    thread mutex_thread1, mutex_thread2;
    mutex_thread1 = thread(&mutex_worker, 1);
    mutex_thread2 = thread(&mutex_worker, 2);
    mutex_thread1.join();
    mutex_thread2.join();
    cout << "Mutex Counter Value: " << plain_shared_cntr << endl << endl;

    return 0;
}

void cea_timer::start() {
   begin = high_resolution_clock::now();
}

double cea_timer::elapsed() {
    end = high_resolution_clock::now();
    double delta = duration<double>(end-begin).count();
    return delta;
}

string cea_timer::elapsed_in_string(int precision) {
    stringstream ss;
    ss << fixed << setprecision(precision) << elapsed() << " sec";
    return ss.str();
}
