#ifndef __MUL_H
#define __MUL_H

#include "num/number.h"

namespace big
{

    template<typename T>
    concept SmallUInt = UInt<T> && not std::is_same<T, std::uint64_t>::value;

    template<SmallUInt T>
    inline num_t<T> operator*(const num_t<T> &, const num_t<T> &);

    //////////////// IMPLEMENTATIONS ////////////////

    /* assumes out length is at least one more than the input length
     * and also that out does not overlap with input
     * adds product of input with scalar to value already present in output
     * returns true if last digit carries (and could not be handled)
     */
    template<SmallUInt T, bool safe=true>
    inline bool muladd(const digit_range<T> &out, const const_digit_range<T> &input, const T scalar)
    {
        using std::uint64_t;
        constexpr size_t down = 64 - bitlen<T>;
        uint64_t scalar_ext = static_cast<uint64_t>(scalar);
        uint64_t spill = 0;
        
        auto oit = out.begin();
        for (auto it = input.begin(); it != input.end(); ++it, ++oit)
        {
            uint64_t res = static_cast<uint64_t>(*oit) + static_cast<uint64_t>(*it) * scalar_ext + spill;
            *oit = static_cast<T>(res);
            spill = res >> down;
        }
        if (spill)
        {
            T old = *oit;
            *oit += static_cast<T>(spill); // spill always fits in T
            if (*oit < old)
            {
                ++oit;
                if constexpr (safe)
                {
                    if (oit == out.end())
                    {
                        return true;
                    }
                }
                return ++*oit == 0;
            }
        }
        return false;
    }


    /* assumes out length is at least the sum of the input lengths,
     * and that out does not overlap with the inputs
     */
    template<SmallUInt T>
    inline void mul(const digit_range<T> &out, const const_digit_range<T> &lhs, const const_digit_range<T> &rhs)
    {
        auto rit = rhs.begin();
        digit_range<T> owindow(out);
        for (; rit != rhs.end(); ++rit, ++owindow.begin_)
        {
            // no need for safe checks because out is assumed to be large enough
            muladd<T, false>(owindow, lhs, *rit);
        }
    }

    template<SmallUInt T>
    num_t<T> operator*(const num_t<T> &lhs, const num_t<T> &rhs)
    {
        if (lhs.value.empty() || rhs.value.empty())
        {
            return 0;
        }
        std::vector<T> out(lhs.value.size() + rhs.value.size());
        mul<T>(digit_range<T>(out), const_digit_range<T>(lhs.value), const_digit_range<T>(rhs.value));
        DB({ num_t<T> res { out }; cerr << lhs.str(true) << " * " << rhs.str(true) << " == " << res.str(true) << endl; });
        return { out };
    }

} // namespace big

#endif//__MUL_H
