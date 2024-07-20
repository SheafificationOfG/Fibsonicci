#include "fib_base.h"

number fibonacci(number n)
{
    if (n <= 1)
    {
        return n;
    }
    return fibonacci(n-1) + fibonacci(n-2);
}
