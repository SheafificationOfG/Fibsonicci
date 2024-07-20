#include "num/mulnum_fft.h"
#include "fib_base.h"

using num = big::num_t<std::uint8_t>;

struct Zrt5
{
    // a + b\sqrt{5}
    num a, b;
    
    Zrt5 operator *(const Zrt5 &x)
    {
        num bb = b * x.b;
        bb = (bb << 2) + bb;
        return Zrt5(
                a * x.a + bb,
                a * x.b + b * x.a);
    }

    Zrt5 &operator*=(Zrt5 const &x)
    {
        return *this = *this * x;
    }

    Zrt5 &operator>>=(size_t n)
    {
        a >>= n;
        b >>= n;
        return *this;
    }
};

number fibonacci(number n)
{
    if (n.value.empty())
    {
        return n;
    }

    Zrt5 step(1, 1);
    Zrt5 fib(1, 1);
    --n;
    while (n > 0)
    {
        if ((n & 1) != 0)
        {
            fib *= step;
            fib >>= 1;
        }
        step *= step;
        step >>= 1;
        n >>= 1;
    }

    return static_cast<number>(fib.b);
}
