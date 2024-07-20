#include "num/mulnum_simple.h"
#include "fib_base.h"

/* signed wrapper for num */
using num = big::num_t<std::uint32_t>;
struct Int
{
    enum class sign_t
    {
        ZERO,
        POS,
        NEG,
    };

    num abs;
    sign_t sign = sign_t::ZERO;

    Int() {};
    Int(num value, sign_t sign=sign_t::POS) : abs(value), sign(sign)
    {
        if (value.value.empty())
        {
            sign = sign_t::ZERO;
        }
    };

    template<big::UInt T>
    explicit operator big::num_t<T>() const { return static_cast<big::num_t<T>>(abs); }
    sign_t signum() const { return sign; }

    void flip_sign()
    {
        switch(sign)
        {
            case sign_t::POS:
                sign = sign_t::NEG;
                break;
            case sign_t::NEG:
                sign = sign_t::POS;
                break;
            default:
                break;
        }
    }

    std::string str(bool full=false) const
    {
        switch (sign)
        {
            case sign_t::POS:
                return "+" + abs.str(full);
            case sign_t::NEG:
                return "-" + abs.str(full);
            default:
                return "0";
        }
    }

    Int &operator+=(const Int &other)
    {
        if (other.sign == sign_t::ZERO)
        {
            return *this;
        }
        if (sign == sign_t::ZERO)
        {
            return *this = other;
        }

        if (sign == other.sign)
        {
            abs += other.abs;
            return *this;
        }

#ifdef DEBUG
        Int old_this(*this);
#endif

        std::strong_ordering cmp = abs <=> other.abs;
        if (cmp == std::strong_ordering::greater)
        {
            abs -= other.abs;
        }
        else if (cmp == std::strong_ordering::less)
        {
            flip_sign();
            abs = other.abs - abs;
        }
        else
        {
            sign = sign_t::ZERO;
            abs.value.clear();
        }
        DB(cerr << old_this.str(true) << " + " << other.str(true) << " == " << str(true) << endl);
        return *this;
    }

    Int &operator-=(const Int &other)
    {
        return *this += -other;
    }

    Int &operator*=(const Int &other)
    {
        return *this = *this * other;
    }

    friend Int operator-(Int num)
    {
        num.flip_sign();
        return num;
    }

    friend Int operator+(Int lhs, const Int &rhs)
    {
        return lhs += rhs;
    }

    friend Int operator-(Int lhs, const Int &rhs)
    {
        return lhs -= rhs;
    }

    friend Int operator*(Int lhs, const Int &rhs)
    {
        if (lhs.sign == sign_t::ZERO || rhs.sign == sign_t::ZERO)
        {
            return Int();
        }
        return Int(
                lhs.abs * rhs.abs,
                lhs.sign == rhs.sign ? sign_t::POS : sign_t::NEG
                );
    }

};

struct M2x2
{
    Int e00, e01, e10, e11;
    M2x2(int e00, int e01, int e10, int e11)
        : e00(e00)
        , e01(e01)
        , e10(e10)
        , e11(e11)
    {}
    M2x2(Int e00, Int e01, Int e10, Int e11)
        : e00(e00)
        , e01(e01)
        , e10(e10)
        , e11(e11)
    {}
    
    M2x2 operator*(M2x2 const &o)
    {
        /* Strassen multiplication (others exist; see Winograd) */
        Int m0 = (e00 + e11) * (o.e00 + o.e11);
        Int m1 = (e10 + e11) * o.e00;
        Int m2 = e00 * (o.e01 - o.e11);
        Int m3 = e11 * (o.e10 - o.e00);
        Int m4 = (e00 + e01) * o.e11;
        Int m5 = (e10 - e00) * (o.e00 + o.e01);
        Int m6 = (e01 - e11) * (o.e10 + o.e11);
        return M2x2(
                m0 + m3 - m4 + m6, m2 + m4,
                m1 + m3, m0 - m1 + m2 + m5
                );
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
