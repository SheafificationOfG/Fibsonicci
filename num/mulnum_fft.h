#ifndef __MUL_H
#define __MUL_H

#include "num/number.h"

#include <algorithm>
#include <complex>
#include <cmath>

namespace big
{

    inline num_t<std::uint8_t> operator*(const num_t<std::uint8_t> &, const num_t<std::uint8_t> &);

    //////////////// IMPLEMENTATIONS ////////////////

    using real_t = double; // seems to be good enough
    using complex = std::complex<real_t>;
    constexpr complex full_rot = 2 * std::numbers::pi_v<real_t> * complex(0, 1);

    template<bool inv=false>
    inline complex primitive_root(size_t N)
    {
        if constexpr(inv)
        {
            return std::exp(-full_rot / static_cast<complex>(N));
        }
        else
        {
            return std::exp(full_rot / static_cast<complex>(N));
        }
    }

    /* rounds x up to the next power of 2 */
    constexpr size_t pow2ceil(size_t x)
    {
        size_t y;
        do
        {
            y = x;
        }
        while (x &= x-1);
        return y << 1;
    }

    constexpr void inc_rev(size_t &x, size_t top_bit)
    {
        while (x & (top_bit >>= 1))
        {
            x ^= top_bit;
        }
        x |= top_bit;
    }

    template<typename T>
    inline std::vector<complex> bit_reverse_shuffle(const std::vector<T> &x, size_t pow2size)
    {
        std::vector<complex> out(pow2size);

        for (size_t i = 0, ri = 0; i < x.size(); ++i, inc_rev(ri, pow2size))
        {
            out[ri] = complex(x[i]);
        }
        return out;
    }

    inline std::vector<std::uint64_t> from_complex(const std::vector<complex> &x)
    {
        using std::uint64_t;
        size_t top_bit = 1;
        while (top_bit < x.size())
        {
            top_bit <<= 1;
        }
        std::vector<uint64_t> out;
        out.reserve(x.size());
       
        for (const auto &xi : x)
        {
            out.emplace_back(static_cast<uint64_t>(std::round(xi.real())));
        }
        return out;
    }

    inline std::vector<std::uint8_t> fold(const std::vector<std::uint64_t> &x)
    {
        using std::uint8_t;
        using std::uint64_t;
        uint64_t spill = 0;
        std::vector<uint8_t> out;
        out.reserve(x.size()+8);
        for (const auto &xi : x)
        {
            uint64_t sum = xi + spill;
            out.emplace_back(static_cast<uint8_t>(sum));
            spill = sum >> 8;
        }
        while (spill)
        {
            out.emplace_back(static_cast<uint8_t>(spill));
            spill >>= 8;
        }
        return out;
    }

    enum class dft_t
    {
        normal = 0,
        inverse = 1
    };

    /* assumes x is already bit-reverse-shuffled
     *
     * [Cooley-Tukey; yoinked from Wikipedia]
     */
    template<dft_t dft_type=dft_t::normal>
    inline void fft(std::vector<complex> &x)
    {
        for (size_t m = 2; m <= x.size(); m <<= 1)
        {
            complex omega = primitive_root<dft_type==dft_t::normal>(m);
            for (size_t k = 0; k < x.size(); k += m)
            {
                complex coef(1);
                size_t m2 = m >> 1;
                for (size_t j = 0; j < m2; ++j)
                {
                    complex t = coef * x[k + j + m2];
                    complex u = x[k + j];
                    x[k + j] = u + t;
                    x[k + j + m2] = u - t;
                    coef *= omega;
                }
            }
        }
        if constexpr(dft_type == dft_t::inverse)
        {
            for (auto &xi : x)
            {
                xi /= x.size();
            }
        }
    }

    num_t<std::uint8_t> operator*(const num_t<std::uint8_t> &lhs, const num_t<std::uint8_t> &rhs)
    {
        size_t size = pow2ceil(std::max(lhs.value.size(), rhs.value.size()) << 1);
        std::vector<complex> lc = bit_reverse_shuffle(lhs.value, size);
        std::vector<complex> rc = bit_reverse_shuffle(rhs.value, size);

        fft(lc);
        fft(rc);

        for (size_t i = 0; i < size; ++i)
        {
            lc[i] *= rc[i];
        }

        std::vector<complex> conv = bit_reverse_shuffle(lc, size);
        fft<dft_t::inverse>(conv);

        DB({ num_t z(fold(from_complex(conv))); cerr << lhs.str(true) << " * " << rhs.str(true) << " == " << z.str(true) << endl; });
        return num_t(fold(from_complex(conv)));
    }
    
} // namespace big


#endif//__MUL_H
