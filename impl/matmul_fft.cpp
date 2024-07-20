#include "num/mulnum_fft.h"
#include "fib_base.h"

using num = big::num_t<std::uint8_t>;

struct M2x2
{
    num e00, e01, e10, e11;
    M2x2(num e00, num e01, num e10, num e11)
        : e00(e00)
        , e01(e01)
        , e10(e10)
        , e11(e11)
    {}
    
    M2x2 operator*(M2x2 const &o)
    {
        return M2x2(
                e00*o.e00 + e01*o.e10,
                e00*o.e01 + e01*o.e11,
                e10*o.e00 + e11*o.e10,
                e10*o.e01 + e11*o.e11);
    }
    M2x2 &operator*=(M2x2 const &o)
    {
        return *this = *this * o;
    }
};

number fibonacci(number n)
{
    M2x2 step(0, 1, 1, 1);
    M2x2 fib(step);
    while (n > 0)
    {
        if ((n & 1) != 0)
        {
            fib *= step;
        }
        step *= step;
        n >>= 1;
    }
    return static_cast<number>(fib.e00);
}
