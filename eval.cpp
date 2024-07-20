#include <atomic>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <optional>
#include <thread>
#include <pthread.h> /* for pthread_cancel */

#include "fib_base.h"

#ifndef LIMIT
#define LIMIT 1.0
#endif

using sec_t = std::chrono::duration<double>;
using std::uint64_t;

const sec_t soft_limit(LIMIT * 1.5);
const sec_t hard_limit(LIMIT);
const sec_t nap(0.01);

constexpr uint64_t first_mark = 92; // 93rd is the largest 64-bit fibonacci number
constexpr uint64_t second_mark = 512; // 512th fibonacci number has 355 bits

bool eval(number n, sec_t &runtime, number &result, const sec_t sleep_dur)
{
    std::atomic<bool> done(false);
    std::atomic<sec_t> run_atomic;
    std::atomic<bool> detached(false);

    sched_param param { sched_get_priority_max(SCHED_FIFO) };
    std::thread runner([&done, &detached, &run_atomic, &result, &n]
            {
                number input = n;
                auto start = std::chrono::steady_clock::now();
                result = fibonacci(n);
                auto delta = std::chrono::steady_clock::now() - start;
                if (n == input)
                {
                    run_atomic = delta;
                    done = true;
                }
            });
    pthread_setschedparam(runner.native_handle(), SCHED_FIFO, &param);

    auto start = std::chrono::steady_clock::now();
    do
    {
        if (done)
        {
            runner.join();
            runtime = run_atomic;
            return true;
        }
        std::this_thread::sleep_for(sleep_dur);
    }
    while (std::chrono::steady_clock::now() - start < soft_limit);
    
    // timeout (don't let this happen too much)
    pthread_cancel(runner.native_handle());
    runner.detach();
    return false;
}

void print_result(number n, sec_t runtime, number result)
{
    std::cout << std::right << std::setw(15) << n.str() << " :: "
        << std::left << std::setw(20) << result.str() << " :: "
        << std::setprecision(5) << runtime.count() << std::endl;
}

int main(int, char *argv[])
{
    number cur = 0;
    sec_t runtime;
    number result;
    number best;

    // check correctness first (against linear implementation)
    {
        uint64_t a = 0, b = 1, tmp;
        while (cur < first_mark && eval(cur, runtime, result, nap))
        {
            if (result != a)
            {
                std::cerr << "ERROR: Output fails to compute term " << cur.str() << ":\n"
                    << "\tExpected: " << a << '\n'
                    << "\tReceived: " << result.str(true) << std::endl;
                return -1;
            }

            print_result(cur, runtime, result);

            if (runtime > soft_limit)
            {
                break;
            }

            if (runtime <= hard_limit)
            {
                best = cur;
            }

            tmp = a + b;
            a = b;
            b = tmp;

            ++cur;
        }

        if (cur == first_mark)
        {
            // check second_mark
            number aa(a);
            number bb(b);
            number tt;
            while (cur <= second_mark && eval(cur, runtime, result, nap))
            {
                if (result != aa)
                {
                    std::cerr << "ERROR: Output fails to compute term " << cur.str() << ":\n"
                        << "\tExpected: " << aa.str(true) << "\n"
                        << "\tReceived: " << result.str(true) << "\n";
                    return -1;
                }
                print_result(cur, runtime, result);

                if (runtime > soft_limit)
                {
                    break;
                }

                if (runtime <= hard_limit)
                {
                    best = cur;
                }

                tt = aa + bb;
                aa = bb;
                bb = tt;

                ++cur;
            }
        }
    }

    // search for upper bound
    while (eval(cur, runtime, result, nap))
    {
        cur += (cur >> 1) - (cur >> 3); // geometric growth
    }

    // now that we have an upper bound, generate ~1k evenly-spaced samples
    number delta = cur >> 10;
    if (delta == 0)
    {
        delta = 1;
    }

    
    if (cur >= second_mark)
    {
        number n = second_mark + 1;
        do
        {
            bool success = eval(n, runtime, result, nap);

            if (!success || runtime > soft_limit)
            {
                break;
            }
            print_result(n, runtime, result);
            if (runtime <= hard_limit)
            {
                best = n;
            }
            n += delta;
        }
        while (1);
    }
    std::cerr << argv[0] << " final result: " << best.str() << '\n';

    number obtained = fibonacci(best);
    std::cerr << "Fibonacci number obtained: " << obtained.str(obtained.value.size() < 10) << std::endl;
    return 0;
}
