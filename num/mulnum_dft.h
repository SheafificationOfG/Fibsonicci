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

    using real_t = double;
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

    inline std::vector<complex> to_complex(const std::vector<std::uint8_t> &x)
    {
        std::vector<complex> out;
        out.reserve(x.size());

        for (const auto &xi : x)
        {
            out.emplace_back(complex(xi));
        }
        return out;
    }

    inline std::vector<std::uint64_t> from_complex(const std::vector<complex> &x)
    {
        using std::uint64_t;
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

    template<dft_t dft_type=dft_t::normal>
    inline std::vector<complex> dft(const std::vector<complex> &x)
    {
        std::vector<complex> f;
        f.reserve(x.size());
        complex primitive = primitive_root<dft_type==dft_t::normal>(x.size());
        complex omega(1);
        for (size_t k = 0; k < x.size(); ++k)
        {
            complex coef(1);
            complex sum;
            for (size_t n = 0; n < x.size(); ++n)
            {
                sum += coef * x[n];
                coef *= omega;
            }

            if constexpr(dft_type == dft_t::normal)
            {
                f.push_back(sum);
            }
            else
            {
                f.push_back(sum / static_cast<complex>(x.size()));
            }
            omega *= primitive;
        }
        return f;
    }

    num_t<std::uint8_t> operator*(const num_t<std::uint8_t> &lhs, const num_t<std::uint8_t> &rhs)
    {
        std::vector<complex> lc = to_complex(lhs.value);
        std::vector<complex> rc = to_complex(rhs.value);

        size_t size = std::max(lhs.value.size(), rhs.value.size()) << 1;

        lc.resize(size);
        rc.resize(size);

        std::vector<complex> lh = dft(lc);
        std::vector<complex> rh = dft(rc);

        for (size_t i = 0; i < lh.size(); ++i)
        {
            lh[i] *= rh[i];
        }

        DB({ num_t z(fold(from_complex(dft<dft_t::inverse>(lh)))); cerr << lhs.str(true) << " * " << rhs.str(true) << " == " << z.str(true) << endl; });
        return num_t(fold(from_complex(dft<dft_t::inverse>(lh))));
    }
    
} // namespace big


#endif//__MUL_H
