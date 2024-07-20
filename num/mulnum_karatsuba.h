#ifndef __MUL_H
#define __MUL_H

#include "num/number.h"

#include <algorithm>

namespace big
{

    template<typename T>
    concept SmallUInt = UInt<T> && not std::is_same<T, std::uint64_t>::value;

    template<SmallUInt T>
    inline num_t<T> operator*(const num_t<T> &, const num_t<T> &);


    //////////////// IMPLEMENTATIONS ////////////////

    /* assumes out length is at least one more than the input length
     * and also that out does not overlap with input
     */
    template<SmallUInt T>
    inline void mul_scalar(const digit_range<T> &out, const const_digit_range<T> &input, const T scalar)
    {
        using std::uint64_t;
        constexpr size_t down = 64 - bitlen<T>;
        uint64_t scalar_ext = static_cast<uint64_t>(scalar);
        uint64_t spill = 0;
        
        auto oit = out.begin();
        for (auto it = input.begin(); it != input.end(); ++it, ++oit)
        {
            uint64_t res = static_cast<uint64_t>(*it) * scalar_ext + spill;
            *oit = static_cast<T>(res);
            spill = res >> down;
        }
        if (spill)
        {
            *oit = static_cast<T>(spill); // spill always fits in T
        }
    }

    /* assumes out length is more than twice the max input length, +2
     * and that out does not overlap with the inputs
     *
     * assumes also that the scratch space is at least six times the 
     */
    template<SmallUInt T, bool cleanup=false>
    inline void mul(const digit_range<T> &out, const const_digit_range<T> &lhs, const const_digit_range<T> &rhs, const digit_range<T> &scratch)
    {
        if (lhs.empty() || rhs.empty())
        {
            return;
        }
        if (rhs.size() == 1)
        {
            mul_scalar<T>(out, lhs, *rhs.begin());
            return;
        }
        if (lhs.size() == 1)
        {
            mul_scalar<T>(out, rhs, *lhs.begin());
            return;
        }

        size_t halfsize = (std::max(lhs.size(), rhs.size())+1) >> 1;
        auto lhs_hit = lhs.size() >= halfsize ?
            std::next(lhs.begin(), halfsize) : lhs.end();
        auto rhs_hit = rhs.size() >= halfsize ?
            std::next(rhs.begin(), halfsize) : rhs.end();

        const_digit_range<T> lhs_lower(lhs.begin(), lhs_hit);
        const_digit_range<T> lhs_upper(lhs_hit, lhs.end());
        const_digit_range<T> rhs_lower(rhs.begin(), rhs_hit);
        const_digit_range<T> rhs_upper(rhs_hit, rhs.end());

        /* scratch memory allocation M[t]:
         * ===============================
         * z3 : (M + 2)[halfsize + 1]
         * z2 : M[halfsize]
         * z0 : M[halfsize] (reused from z2)
         * ===============================
         * M[2h] >= (M + 2)[h] + (M + 2)
         * (where h >= 2, since h < 2 requires no scratch space)
         * Therefore, 4M >= 3M + 6
         * i.e., M >= 6
         */

        auto slo = scratch.begin();
        auto shi = std::next(slo, halfsize);
        digit_range<T> lhs_loup(slo, shi);

        // perform computations
        // z3 = (x0 + x1) * (y0 + y1)
        if (lhs_upper.size() >= lhs_lower.size()
                ? add<T, true>(lhs_loup, lhs_upper, lhs_lower)
                : add<T, true>(lhs_loup, lhs_lower, lhs_upper))
        {
            ++lhs_loup.end_;
            *lhs_loup.rbegin() = 1;
            ++shi;
        }

        slo = shi;
        shi = std::next(slo, halfsize);
        digit_range<T> rhs_loup(slo, shi);
        if (rhs_upper.size() >= rhs_lower.size()
                ? add<T, true>(rhs_loup, rhs_upper, rhs_lower)
                : add<T, true>(rhs_loup, rhs_lower, rhs_upper))
        {
            ++rhs_loup.end_;
            *rhs_loup.rbegin() = 1;
            ++shi;
        }
        
        slo = shi;
        shi = std::next(slo, (std::max(lhs_loup.size(), rhs_loup.size())+1) << 1);
        digit_range<T> z3(slo, shi);
        if constexpr(cleanup)
        {
            for (auto it = z3.begin(); it != z3.end(); ++it)
            {
                *it = 0;
            }
        }

        digit_range<T> workspace(shi, scratch.end());
        mul<T, true>(z3, lhs_loup, rhs_loup, workspace);
        
        // z0 = x0 * y0
        // (we can put z0 in the output first)
        auto out_mid = std::next(out.begin(), halfsize<<1);
        digit_range<T> z0(out.begin(), out_mid);
        mul<T, true>(z0, lhs_lower, rhs_lower, workspace);

        // z2 = x1 * y1
        // (we can also put this in the output, thanks to alignment)
        digit_range<T> z2(out_mid, out.end());
        mul<T, true>(z2, lhs_upper, rhs_upper, workspace);

        // z1 = z3 - z2 - z0
        // no underflows possible
        DB(cerr << num_t<T>(std::vector<T>(z3.begin(), z3.end())).str(true)
                << " - " << num_t<T>(std::vector<T>(z2.begin(), z2.end())).str(true)
                << " == ");
        sub<T>(z3, z3, z2);
        DB(cerr << num_t<T>(std::vector<T>(z3.begin(), z3.end())).str(true) << endl);
        DB(cerr << num_t<T>(std::vector<T>(z3.begin(), z3.end())).str(true)
                << " - " << num_t<T>(std::vector<T>(z0.begin(), z0.end())).str(true)
                << " == ");
        sub<T>(digit_range<T>(z3), const_digit_range<T>(z3), z0);
        DB(cerr << num_t<T>(std::vector<T>(z3.begin(), z3.end())).str(true) << endl);

        // prod = z0 + (z1 << half) + (z2 << half*2)
        //      = (z0 + (z2 << half*2)) + (z1 << half)

        digit_range<T> z0z2_shifted(std::next(out.begin(), halfsize), out.end());
        if (z0z2_shifted.size() >= z3.size())
        {
            add<T>(z0z2_shifted, z0z2_shifted, z3);
        }
        else
        {
            add<T>(z0z2_shifted, z3, z0z2_shifted);
        }
    }

    template<SmallUInt T>
    num_t<T> operator*(const num_t<T> &lhs, const num_t<T> &rhs)
    {
        size_t maxsize = std::max(lhs.value.size(), rhs.value.size());
        std::vector<T> out((maxsize+1) << 1);
        std::vector<T> scratch(out.size() << 3); // conservative allocation for scratch memory
        mul<T>(digit_range<T>(out), const_digit_range<T>(lhs.value), const_digit_range<T>(rhs.value), digit_range<T>(scratch));
        DB({ num_t<T> res { out }; cerr << lhs.str(true) << " * " << rhs.str(true) << " == " << res.str(true) << endl; });
        return { out };
    }

} // namespace big

#endif//__MUL_H
