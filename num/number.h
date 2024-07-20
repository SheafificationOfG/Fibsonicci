/* Helper functions for memory-aware arithmetic
 */

#ifndef __NUM_H
#define __NUM_H

#include <cmath>
#include <climits>
#include <cstdint>
#include <compare>
#include <concepts>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef DEBUG
#include <iostream>
#include <iomanip>
using std::cerr;
using std::endl;
#define DB(X) X
#else
#define DB(X)
#endif

namespace big
{
    
    template<typename T>
    concept UInt = std::unsigned_integral<T>;
    template<typename T>
    concept Int = std::integral<T>;

    using std::size_t;

    template<UInt T>
    constexpr size_t bitlen = CHAR_BIT * sizeof(T);
    template<UInt T>
    constexpr size_t bitlog =
            bitlen<T> ==128 ? 7 :
            bitlen<T> == 64 ? 6 :
            bitlen<T> == 32 ? 5 :
            bitlen<T> == 16 ? 4 : 3;
    

    template<UInt T>
    struct num_t
    {
        using int_t = T;
        std::vector<int_t> value;

        inline num_t() {}

        template<Int I>
        inline num_t(const I);
        inline num_t(const std::string &);

        inline num_t(const std::vector<T> &v) : value(v) { full_reduce(); }
        inline num_t(std::vector<T> &&v) : value(std::move(v)) { full_reduce(); }

        inline num_t &operator++();
        inline num_t &operator--();
        inline num_t operator++(int);
        inline num_t operator--(int);
        inline num_t &operator+=(const num_t &);
        inline num_t &operator-=(const num_t &);
        inline num_t &operator&=(const num_t &);
        inline num_t &operator|=(const num_t &);
        inline num_t &operator<<=(const size_t);
        inline num_t &operator>>=(const size_t);

        template<Int I>
        inline num_t &operator+=(const I);
        template<Int I>
        inline num_t &operator-=(const I);
        template<Int I>
        inline num_t &operator&=(const I);
        template<Int I>
        inline num_t &operator|=(const I);

        explicit inline operator long double() const;
        std::string str(bool=false) const;
        T residue() const;

        inline void reduce_once();
        inline void full_reduce();
        
        template<UInt S>
        explicit inline operator num_t<S>() const;
    };

    using number = num_t<std::uint64_t>;

    template<UInt T>
    inline num_t<T> operator+(num_t<T>, const num_t<T> &);
    template<UInt T>
    inline num_t<T> operator-(num_t<T>, const num_t<T> &);
    template<UInt T>
    inline num_t<T> operator&(num_t<T>, const num_t<T> &);
    template<UInt T>
    inline num_t<T> operator|(num_t<T>, const num_t<T> &);
    template<UInt T>
    inline num_t<T> operator<<(num_t<T>, const size_t);
    template<UInt T>
    inline num_t<T> operator>>(num_t<T>, const size_t);
    template<UInt T>
    inline std::strong_ordering operator<=>(const num_t<T> &, const num_t<T> &);
    template<UInt T>
    inline bool operator==(const num_t<T> &, const num_t<T> &);

    template<UInt T, Int I>
    inline num_t<T> operator+(num_t<T>, const I);
    template<UInt T, Int I>
    inline num_t<T> operator-(num_t<T>, const I);
    template<UInt T, Int I>
    inline num_t<T> operator&(num_t<T>, const I);
    template<UInt T, Int I>
    inline num_t<T> operator|(num_t<T>, const I);
    template<UInt T, Int I>
    inline std::strong_ordering operator<=>(const num_t<T> &, const I);
    template<UInt T, Int I>
    inline bool operator==(const num_t<T> &, const I);

    //////////////// IMPLEMENTATIONS ////////////////

    template<typename Iterator>
    struct range_interface
    {
        using iterator = Iterator;
        iterator begin_;
        iterator end_;
        iterator begin() const { return begin_; }
        iterator end() const { return end_; }
        std::reverse_iterator<iterator> rbegin() const { return std::make_reverse_iterator(end_); }
        std::reverse_iterator<iterator> rend() const { return std::make_reverse_iterator(begin_); }
        size_t size() const { return end_ - begin_; }
        bool empty() const { return end_ == begin_; }
    };

    template<UInt T>
    struct digit_range : range_interface<typename std::vector<T>::iterator>
    {
        digit_range(const std::vector<T>::iterator &begin, const std::vector<T>::iterator &end)
            : range_interface<typename std::vector<T>::iterator>(begin, end)
        {}
        digit_range(std::vector<T> &v)
            : range_interface<typename std::vector<T>::iterator>(v.begin(), v.end())
        {}
        digit_range(std::vector<T> &v, size_t len)
            : range_interface<typename std::vector<T>::iterator>(v.begin(), std::next(v.begin(), len))
        {}
    };

    template<UInt T>
    struct const_digit_range : range_interface<typename std::vector<T>::const_iterator>
    {
        const_digit_range(const std::vector<T>::const_iterator &begin, const std::vector<T>::const_iterator &end)
            : range_interface<typename std::vector<T>::const_iterator>(begin, end)
        {}
        const_digit_range(const std::vector<T> &v)
            : range_interface<typename std::vector<T>::const_iterator>(v.begin(), v.end())
        {}
        const_digit_range(const std::vector<T> &v, size_t len)
            : range_interface<typename std::vector<T>::const_iterator>(v.begin(), std::next(v.begin(), len))
        {}
        const_digit_range(const digit_range<T> &r)
            : range_interface<typename std::vector<T>::const_iterator>(r.begin_, r.end_)
        {}
    };

    /* increments the number, and returns true if the last digit
     * had a carry
     */
    template<UInt T>
    inline bool increment(const digit_range<T> &range)
    {
        bool carry = true;
        for (auto it = range.begin(); carry & it != range.end(); ++it)
        {
            carry = (++*it) == 0;
        }
        return carry;
    }

    /* decrements the number, and returns true if the last digit
     * borrowed
     */
    template<UInt T>
    inline bool decrement(const digit_range<T> &range)
    {
        bool borrow = true;
        for (auto it = range.begin(); borrow & it != range.end(); ++it)
        {
            borrow = (*it)--;
        }
        return borrow;
    }

    /* assumes lengths of arguments descend
     * returns if the last digit carries outside the `out` range
     */
    template<UInt T, bool clear=false>
    inline bool add(const digit_range<T> &out, const const_digit_range<T> &lhs, const const_digit_range<T> &rhs)
    {
        auto oit = out.begin();
        auto lit = lhs.begin();
        auto rit = rhs.begin();
        T carry = false;
        for (; rit != rhs.end(); ++lit, ++rit, ++oit)
        {
            T old = *lit; // in case lhs and out overlap
            *oit = *lit + *rit + carry;
            carry = carry ? *oit <= old : *oit < old;
        }
        for (; carry && lit != lhs.end(); ++lit, ++oit)
        {
            *oit = *lit + 1;
            carry = *oit == 0;
        }
        for (; lit != lhs.end(); ++lit, ++oit)
        {
            *oit = *lit;
        }
        if constexpr(clear)
        {
            if (oit != out.end())
            {
                *oit = carry;
                while (++oit != out.end())
                {
                    *oit = 0;
                }
                return false;
            }
        }
        return carry;
    }

    /* assumes lengths of arguments descend
     * returns if the last digit borrows (and could not be handled by out range)
     */
    template<UInt T>
    inline bool sub(const digit_range<T> &out, const const_digit_range<T> &lhs, const const_digit_range<T> &rhs)
    {
        auto oit = out.begin();
        auto lit = lhs.begin();
        auto rit = rhs.begin();
        bool borrow = false;
        for (; rit != rhs.end(); ++lit, ++rit, ++oit)
        {
            T old = *lit; // in case lhs and out overlap
            *oit = *lit - *rit - borrow;
            borrow = borrow ? *oit >= old : *oit > old;
        }
        for (; borrow && lit != lhs.end(); ++lit, ++oit)
        {
            T old = *lit; // in case lhs and out overlap
            *oit = *lit - 1;
            borrow = *oit >= old;
        }
        for (; lit != lhs.end(); ++lit, ++oit)
        {
            *oit = *lit;
        }
        return borrow;
    }

    /* assumes all arguments have equal lengths */
    template<UInt T>
    inline void bitwise_and(const digit_range<T> &out, const const_digit_range<T> &lhs, const const_digit_range<T> &rhs)
    {
        auto oit = out.begin();
        auto lit = lhs.begin();
        auto rit = rhs.begin();
        for (; lit != lhs.end(); ++lit, ++rit, ++oit)
        {
            *oit = *lit & *rit;
        }
    }

    /* assumes all arguments have equal lengths */
    template<UInt T>
    inline void bitwise_or(const digit_range<T> &out, const const_digit_range<T> &lhs, const const_digit_range<T> &rhs) 
    {
        auto oit = out.begin();
        auto lit = lhs.begin();
        auto rit = rhs.begin();
        for (; lit != lhs.end(); ++lit, ++rit, ++oit)
        {
            *oit = *lit | *rit;
        }
    }

    /* assumes bits <= bitlen<T>, and the ranges have equal length
     * returns the spilled bits from shift
     */
    template<UInt T>
    inline T lshift(const digit_range<T> &out, const const_digit_range<T> &src, size_t bits)
    {
        T spill = 0;
        auto oit = out.begin();
        for (auto it = src.begin(); it != src.end(); ++it, ++oit)
        {
            T next_spill = *it >> (bitlen<T> - bits);
            *oit = (*it << bits) | spill;
            spill = next_spill;
        }
        return spill;
    }

    /* assumes bits <= bitlen<T>, and the ranges have equal length
     * returns the spilled bits
     */
    template<UInt T>
    inline T rshift(const digit_range<T> &out, const const_digit_range<T> &src, size_t bits)
    {
        T spill = 0;
        auto orit = out.rbegin();
        for (auto rit = src.rbegin(); rit != src.rend(); ++rit, ++orit)
        {
            T next_spill = *rit << (bitlen<T> - bits);
            *orit = (*rit >> bits) | spill;
            spill = next_spill;
        }
        return spill;
    }

    template<UInt T, UInt S>
    inline void copy(const digit_range<T> &out, const const_digit_range<S> &in)
    {
        constexpr size_t lenT = bitlen<T>;
        constexpr size_t lenS = bitlen<S>;

        auto oit = out.begin();
        auto it = in.begin();

        if constexpr (lenT >= lenS)
        {
            for (; it != in.end(); ++oit)
            {
                *oit = 0;
                DB(cerr << "# chunk" << endl);
                for (size_t sh = 0; it != in.end() && sh < lenT; sh += lenS, ++it)
                {
                    DB(cerr << "# adding " << std::hex << static_cast<T>(*it) << " shifted by " << std::dec << sh << endl);
                    *oit |= static_cast<T>(*it) << sh;
                    DB(cerr << "# *oit = " << std::hex << static_cast<T>(*oit) << std::dec << endl);
                }
            }
        }
        else
        {
            for (; it != in.end(); ++it)
            {
                for (size_t sh = 0; sh < lenS; sh += lenT, ++oit)
                {
                    *oit = static_cast<T>((*it) >> sh);
                }
            }
        }
    }

    template<UInt T>
    template<Int I>
    num_t<T>::num_t(const I x)
    {
        if constexpr(sizeof(I) <= sizeof(T))
        {
            if (x)
            {
                value = { static_cast<T>(x) };
            }
        }
        else
        {
            using UI = std::make_unsigned_t<I>;
            std::vector<UI> xv { static_cast<UI>(x) };
            value.resize(sizeof(UI)/sizeof(T));
            copy<T, UI>(value, xv);
            full_reduce();
        }
    }
    template<UInt T>
    num_t<T>::num_t(const std::string &digits_str)
    {
        /* reverse double-dabble... half-huff? */
        std::vector<unsigned> digits;
        digits.reserve(digits_str.size());
        for (auto rit = digits_str.rbegin(); rit != digits_str.rend(); ++rit)
        {
            digits.push_back(*rit-'0');
        }

        // efficiency is not the name of the game for this function
        num_t<T> bit(1);
        while (!digits.empty())
        {
            bool new_bit = false;
            for (auto rit = digits.rbegin(); rit != digits.rend(); ++rit)
            {
                if (new_bit)
                {
                    *rit |= 1 << 4;
                }
                new_bit = !!(1 & *rit);
                *rit >>= 1;
                if (*rit >= 8)
                {
                    *rit -= 3;
                }
            }
            while (!digits.empty() && !*digits.rbegin())
            {
                digits.pop_back();
            }
            if (new_bit)
            {
                *this |= bit;
            }
            bit <<= 1;
        }
    }

    template<UInt T>
    num_t<T> &num_t<T>::operator++()
    {
        DB(cerr << str(true) << " + 1 == ");
        if (increment<T>(digit_range<T>(value)))
        {
            value.push_back(1);
        }
        DB(cerr << str(true) << endl);
        return *this;
    }
    template<UInt T>
    num_t<T> &num_t<T>::operator--()
    {
        DB(bool print = !value.empty());
        DB(if (print) { cerr << str(true) << " - 1 == "; });
        decrement<T>(digit_range<T>(value));
        reduce_once();
        DB(if (print) { cerr << str(true) << endl; });
        return *this;
    }
    template<UInt T>
    num_t<T> num_t<T>::operator++(int)
    {
        num_t<T> out(*this);
        ++*this;
        return out;
    }
    template<UInt T>
    num_t<T> num_t<T>::operator--(int)
    {
        num_t<T> out(*this);
        --*this;
        return out;
    }
    template<UInt T>
    num_t<T> &num_t<T>::operator+=(const num_t<T> &other)
    {
        if (value.size() < other.value.size())
        {
            value.resize(other.value.size());
        }
        if (add<T>(digit_range<T>(value), const_digit_range<T>(value), const_digit_range<T>(other.value)))
        {
            value.push_back(1);
        }
        DB(cerr << str(true) << endl);
        return *this;
    }
    template<UInt T>
    num_t<T> &num_t<T>::operator-=(const num_t<T> &other)
    {
        DB(bool print = *this >= other);
        DB(if (print) { cerr << str(true) << " - " << other.str(true) << " == "; });
        if (value.size() < other.value.size())
        {
            value.resize(other.value.size());
        }
        sub<T>(digit_range<T>(value), const_digit_range<T>(value), const_digit_range<T>(other.value));
        full_reduce();
        DB(if (print) { cerr << str(true) << endl; } );
        return *this;
    }
    template<UInt T>
    num_t<T> &num_t<T>::operator&=(const num_t<T> &other)
    {
        DB(cerr << str(true) << " & " << other.str(true) << " == ");
        if (value.size() > other.value.size())
        {
            value.resize(other.value.size());
        }
        bitwise_and<T>(digit_range<T>(value), const_digit_range<T>(other.value), const_digit_range<T>(value));
        full_reduce();
        DB(cerr << str(true) << endl);
        return *this;
    }
    template<UInt T>
    num_t<T> &num_t<T>::operator|=(const num_t<T> &other)
    {
        DB(cerr << str(true) << " | " << other.str(true) << " == ");
        if (value.size() < other.value.size())
        {
            value.resize(other.value.size());
        }
        bitwise_or<T>(digit_range<T>(value), const_digit_range<T>(value), const_digit_range<T>(other.value));
        DB(cerr << str(true) << endl);
        return *this;
    }
    template<UInt T>
    num_t<T> &num_t<T>::operator<<=(const size_t n)
    {
        DB(cerr << str(true) << " << " << n << " == ");
        size_t whole_shift = n >> bitlog<T>;
        size_t partial_shift = n & ((static_cast<T>(1) << bitlog<T>) - 1);
        value.insert(value.begin(), whole_shift, 0);

        digit_range<T> vshifted(std::next(value.begin(), whole_shift), value.end());

        T spill = lshift<T>(vshifted, vshifted, partial_shift);
        if (spill)
        {
            value.push_back(spill);
        }
        DB(cerr << str(true) << endl);
        return *this;
    }
    template<UInt T>
    num_t<T> &num_t<T>::operator>>=(const size_t n)
    {
        DB(cerr << str(true) << " >> " << n << " == ");
        size_t whole_shift = n >> bitlog<T>;
        if (value.size() <= whole_shift)
        {
            value.clear();
            DB(cerr << "0" << endl);
            return *this;
        }
        value.erase(value.begin(), std::next(value.begin(), whole_shift));

        size_t partial_shift = n & ((static_cast<T>(1) << bitlog<T>) - 1);
        rshift<T>(digit_range<T>(value), const_digit_range<T>(value), partial_shift);
        reduce_once();
        DB(cerr << str(true) << endl);
        return *this;
    }

    template<UInt T>
    template<Int I>
    num_t<T> &num_t<T>::operator+=(const I n)
    {
        return *this += num_t<T>(n);
    }
    template<UInt T>
    template<Int I>
    num_t<T> &num_t<T>::operator-=(const I n)
    {
        return *this -= num_t<T>(n);
    }
    template<UInt T>
    template<Int I>
    num_t<T> &num_t<T>::operator&=(const I n)
    {
        return *this &= num_t<T>(n);
    }
    template<UInt T>
    template<Int I>
    num_t<T> &num_t<T>::operator|=(const I n)
    {
        return *this |= num_t<T>(n);
    }

    template<UInt T>
    num_t<T>::operator long double() const
    {
        long double out = 0.l;
        constexpr long double shift = std::pow(2.l, static_cast<long double>(bitlen<T>));

        for (auto rit = value.rbegin(); rit != value.rend(); ++rit)
        {
            out *= shift;
            out += static_cast<long double>(*rit);
        }
        return out;
    }
    template<UInt T>
    std::string num_t<T>::str(bool full) const
    {
        if (value.empty()) { return "0"; }

        constexpr T fullmask = -static_cast<T>(1);
        constexpr T top_bit = fullmask ^ (fullmask >> 1);

        // double-dabble
        std::vector<unsigned> digits;

        size_t offset = 0;
        for (auto rit = value.rbegin(); rit != value.rend(); ++rit)
        {
            for (T bit = top_bit; bit; bit >>= 1)
            {
                bool new_bit = *rit & bit;
                for (unsigned &digit : digits)
                {
                    if (digit >= 5)
                    {
                        digit += 3;
                    }

                    digit <<= 1;
                    if (new_bit)
                    {
                        digit |= 1;
                    }
                    if ((new_bit = (digit > 0xfu)))
                    {
                        digit &= 0xfu;
                    }
                }
                if (new_bit)
                {
                    digits.push_back(1);
                }
            }
            if (!full && digits.size() > 32)
            {
                size_t to_erase = digits.size() - 32;
                offset += to_erase;
                digits.erase(digits.begin(), std::next(digits.begin(), to_erase));
            }
        }

        // now print the digits
        std::stringstream ss;
        size_t num_digits = digits.size() + offset;
        auto rit = digits.rbegin();

        if (!full && num_digits > 10)
        {
            ss << *rit << '.';
            size_t counter = 10;
            for (++rit; --counter; ++rit)
            {
                ss << *rit;
            }
            ss << "e+" << (num_digits-1);
            return ss.str();
        }
        for (; rit != digits.rend(); ++rit)
        {
            ss << *rit;
        }
        return ss.str();
    }
    template<UInt T>
    T num_t<T>::residue() const
    {
        return value.empty() ? 0 : value[0];
    }

    template<UInt T>
    void num_t<T>::reduce_once()
    {
        if (!value.empty() && !*value.rbegin())
        {
            value.pop_back();
        }
    }
    template<UInt T>
    void num_t<T>::full_reduce()
    {
        while (!value.empty() && !*value.rbegin())
        {
            value.pop_back();
        }
    }

    template<UInt T>
    template<UInt S>
    num_t<T>::operator num_t<S>() const
    {
        constexpr size_t lenT = bitlen<T>;
        constexpr size_t lenS = bitlen<S>;
        if constexpr(lenT > lenS)
        {
            std::vector<S> out(value.size() * lenT / lenS);
            copy<S, T>(digit_range<S>(out), const_digit_range<T>(value));
            num_t<S> res { out };
            if constexpr(lenT / lenS == 2)
            {
                res.reduce_once();
            }
            else
            {
                res.full_reduce();
            }
            DB(cerr << str(true) << " == " << res.str(true) << " # " << bitlen<T> << " -> " << bitlen<S> << endl);
            return res;
        }
        else
        {
            std::vector<S> out((value.size() * lenT + lenS - 1) / lenS);
            copy<S, T>(digit_range<S>(out), const_digit_range<T>(value));
            DB({ num_t<S> res { out }; cerr << str(true) << " == " << res.str(true) << " # " << bitlen<T> << " -> " << bitlen<S> << endl; });
            return { out };
        }
    }

    template<UInt T>
    num_t<T> operator+(num_t<T> lhs, const num_t<T> &rhs)
    {
        return lhs += rhs;
    }
    template<UInt T>
    num_t<T> operator-(num_t<T> lhs, const num_t<T> &rhs)
    {
        return lhs -= rhs;
    }
    template<UInt T>
    num_t<T> operator&(num_t<T> lhs, const num_t<T> &rhs)
    {
        return lhs &= rhs;
    }
    template<UInt T>
    num_t<T> operator|(num_t<T> lhs, const num_t<T> &rhs)
    {
        return lhs |= rhs;
    }
    template<UInt T>
    num_t<T> operator<<(num_t<T> lhs, const size_t n)
    {
        return lhs <<= n;
    }
    template<UInt T>
    num_t<T> operator>>(num_t<T> lhs, const size_t n)
    {
        return lhs >>= n;
    }

    template<UInt T>
    std::strong_ordering operator<=>(const num_t<T> &lhs, const num_t<T> &rhs)
    {
        std::strong_ordering cmp = lhs.value.size() <=> rhs.value.size();
        if (cmp != std::strong_ordering::equal)
        {
            return cmp;
        }
        auto lit = lhs.value.rbegin();
        auto rit = rhs.value.rbegin();
        for (; lit != lhs.value.rend(); ++lit, ++rit)
        {
            cmp = *lit <=> *rit;
            if (cmp != std::strong_ordering::equal)
            {
                return cmp;
            }
        }
        return std::strong_ordering::equal;
    }
    template<UInt T>
    bool operator==(const num_t<T> &lhs, const num_t<T> &rhs)
    {
        auto cmp = lhs <=> rhs;
        return cmp == std::strong_ordering::equal;
    }

    template<UInt T, Int I>
    num_t<T> operator+(num_t<T> lhs, const I n)
    {
        return lhs += n;
    }
    template<UInt T, Int I>
    num_t<T> operator-(num_t<T> lhs, const I n)
    {
        return lhs -= n;
    }
    template<UInt T, Int I>
    num_t<T> operator&(num_t<T> lhs, const I n)
    {
        return lhs &= n;
    }
    template<UInt T, Int I>
    num_t<T> operator|(num_t<T> lhs, const I n)
    {
        return lhs |= n;
    }

    template<UInt T, Int I>
    std::strong_ordering operator<=>(const num_t<T> &lhs, const I n)
    {
        return lhs <=> num_t<T>(n);
    }
    template<UInt T, Int I>
    bool operator==(const num_t<T> &lhs, const I n)
    {
        return lhs == num_t<T>(n);
    }

} // namespace big

#endif//__NUM_H
