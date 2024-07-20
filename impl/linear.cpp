#include "fib_base.h"

number fibonacci(number n)
{
    number a(0);
    number b(1);
    number tmp;
    while (n-- > 0)
    {
        tmp = a + b;
        a = b;
        b = tmp;
    }
    return a;
}
