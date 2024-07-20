#include <chrono>
#include <iostream>
#include <string>

#include "fib_base.h"

using sec_t = std::chrono::duration<double>;


int main()
{
    std::string index_str;
    std::cin >> index_str;
    number index(index_str);

#ifndef CHECK
    std::cout << "Computing F_" << index.str(true) << " in " << std::flush;
    auto start = std::chrono::steady_clock::now();
#endif

    number fib = fibonacci(index);

#ifndef CHECK
    sec_t delta = std::chrono::steady_clock::now() - start;
    std::cout << delta << std::endl;
#endif


#ifdef CHECK
    for (auto rit = fib.value.rbegin(); rit != fib.value.rend(); ++rit)
    {
        std::cout << std::hex << *rit;
    }
    std::cout << std::endl;
#elif !defined(PERF)
    std::string decimal = fib.str(false);
    std::cout << "Result: " << decimal << std::endl;

    if (decimal.contains('e'))
    {
        char c;
        do
        {
            std::cout << "Fully expand? [y/n] " << std::flush;
            std::cin >> c;
            if (c == 'n' || c == 'N')
            {
                break;
            }
            if (c == 'y' || c == 'Y')
            {
                std::cout << fib.str(true) << std::endl;
                break;
            }
        }
        while (1);
    }
#endif

    return 0;
}
