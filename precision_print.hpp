#ifndef PRECISION_PRINT_HPP_INCLUDE
#define PRECISION_PRINT_HPP_INCLUDE

#include <clocale>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace prprint {

enum class Rounding : unsigned char {
    keep,
    upward,
    downward,
    toNearest,
    towardZero
};

struct PrPrint {
    explicit PrPrint(unsigned short precision_, bool trimZeros_ = false, Rounding roundMode_ = Rounding::keep)
        : precision(precision_),
          trimZeros(trimZeros_),
          roundMode(roundMode_) {}

    unsigned short precision;
    bool trimZeros;
    Rounding roundMode;
};

namespace detail {

    template <bool>
    struct TrimZerosTag {};

    using iosb = std::ios_base;

    template <class CharT, class Traits, class T>
    inline void printer(std::basic_ostream<CharT, Traits>& os, PrPrint p, T num, TrimZerosTag<false>) {
        const auto prevPrecision = os.precision(p.precision);
        const auto prevFmtflags = os.setf(iosb::fixed, iosb::floatfield);
        os.unsetf(iosb::adjustfield);
        os << num;
        os.flags(prevFmtflags);
        os.precision(prevPrecision);
    }

    template <class CharT, class Traits, class T>
    inline void printer(std::basic_ostream<CharT, Traits>& os, PrPrint p, T num, TrimZerosTag<true>) {
        const auto osFlags = os.flags();
        const std::locale loc(os.getloc());

        std::basic_ostringstream<CharT, Traits> sstr;
        sstr.precision(p.precision);
        sstr.flags(iosb::fixed | (osFlags & (iosb::showpos | iosb::showpoint | iosb::uppercase)));
        sstr.imbue(loc);

        sstr << num;
        std::basic_string<CharT, Traits> s(sstr.str());

        if (p.precision != 0u) {
            s.erase(s.find_last_not_of(static_cast<CharT>('0')) + 1);
            if (s.back() == std::use_facet<std::numpunct<CharT>>(loc).decimal_point() &&
                !(osFlags & iosb::showpoint)) s.pop_back();
        }
        os << s;
    }

    inline void printfFmtFloat(char* fPtr, const iosb::fmtflags osFlags) {
        *fPtr = '%';
        if (osFlags & iosb::showpos) *++fPtr = '+';
        if (osFlags & iosb::showpoint) *++fPtr = '#';
        *++fPtr = '.';
        *++fPtr = '*';
        if (osFlags & iosb::uppercase) *++fPtr = 'F';
        else *++fPtr = 'f';
        *++fPtr = '\0';
    }

    template <class CharT, class Traits>
    inline void applyLocaleFmt(std::basic_string<CharT, Traits>& s,
                               const std::locale& loc,
                               const CharT decimalPoint) {
        if (s.size() <= 1) return;
        const auto pointPos = s.find(decimalPoint, 1);

        constexpr auto bstring_npos = std::basic_string<CharT, Traits>::npos;
        const auto& numPunct = std::use_facet<std::numpunct<CharT>>(loc);
        const CharT sep = numPunct.thousands_sep();
        const CharT point = numPunct.decimal_point();
        const std::string grouping(numPunct.grouping());

        typename std::basic_string<CharT, Traits>::size_type digits;
        if (pointPos != bstring_npos) {
            s[pointPos] = point;
            digits = pointPos;
        }
        else digits = s.size();

        if (grouping.empty()) return;

        if (s[0] == static_cast<CharT>('-') ||
            s[0] == static_cast<CharT>('+')) --digits;

        unsigned sepsTotal = 0;
        auto gItr = grouping.begin();
        const auto gLast = grouping.end() - 1;
        while (digits > static_cast<std::size_t>(*gItr)) {
            digits -= static_cast<std::size_t>(*gItr);
            ++sepsTotal;
            if (gItr != gLast) ++gItr;
        }
        if (sepsTotal == 0) return;

        std::basic_string<CharT, Traits> result(s.size() + sepsTotal, static_cast<CharT>(0));

        typename std::basic_string<CharT, Traits>::size_type last;
        typename std::basic_string<CharT, Traits>::const_iterator sItr;
        if (pointPos != bstring_npos) {
            last = pointPos + sepsTotal;
            sItr = s.begin() + pointPos;
            result.replace(last, bstring_npos, s, pointPos, bstring_npos);
        }
        else {
            last = result.size();
            sItr = s.end();
        }

        gItr = grouping.begin();
        auto groupCount = static_cast<std::size_t>(*gItr);
        do {
            result[--last] = *--sItr;
            if (--groupCount == 0) {
                if (last <= 1) break;
                result[--last] = sep;
                if (gItr != gLast) ++gItr;
                groupCount = static_cast<std::size_t>(*gItr);
            }
        } while (last > 1);
        result[--last] = *--sItr;

        s = std::move(result);
    }

    template <class T>
    inline void printer(std::ostream& os, PrPrint p, T num, TrimZerosTag<true>) {
        const auto osFlags = os.flags();
        const std::locale loc(os.getloc());
        const char decimalPoint = std::localeconv()->decimal_point[0];

        char format[8];
        printfFmtFloat(format, osFlags);

        const int len = std::snprintf(nullptr, 0, format, p.precision, num) + 1;
        std::string s(len, 0);
        std::snprintf(&s[0], len, format, p.precision, num);
        s.pop_back();

        if (p.precision != 0u) {
            s.erase(s.find_last_not_of('0') + 1);
            if (s.back() == decimalPoint && !(osFlags & iosb::showpoint)) s.pop_back();
        }

        applyLocaleFmt(s, loc, decimalPoint);
        os << s;
    }

    template <class T>
    inline void printer(std::wostream& os, PrPrint p, T num, TrimZerosTag<true>) {
        const auto osFlags = os.flags();
        const std::locale loc(os.getloc());
        const wchar_t decimalPoint = std::btowc(std::localeconv()->decimal_point[0]);

        char format[8];
        printfFmtFloat(format, osFlags);

        wchar_t wformat[8];
        const char* fPtrBegin = format;
        std::mbstate_t mbstate;
        std::mbsrtowcs(wformat, &fPtrBegin, 8, &mbstate);

        const int len = std::snprintf(nullptr, 0, format, p.precision, num) + 1;
        std::wstring s(len, 0);
        std::swprintf(&s[0], len, wformat, p.precision, num);
        s.pop_back();

        if (p.precision != 0u) {
            s.erase(s.find_last_not_of(L'0') + 1);
            if (s.back() == decimalPoint && !(osFlags & iosb::showpoint)) s.pop_back();
        }

        applyLocaleFmt(s, loc, decimalPoint);
        os << s;
    }

    template <class CharT, class Traits, class T>
    inline void printer(std::basic_ostream<CharT, Traits>& os, PrPrint p, T num) {
        const auto tenPowP = std::pow(10.0, p.precision);
        switch (p.roundMode) {
        case Rounding::keep:
            break;
        case Rounding::upward:
            num = std::ceil(num * tenPowP) / tenPowP;
            break;
        case Rounding::downward:
            num = std::floor(num * tenPowP) / tenPowP;
            break;
        case Rounding::toNearest:
            num = std::round(num * tenPowP) / tenPowP;
            break;
        case Rounding::towardZero:
            num = std::trunc(num * tenPowP) / tenPowP;
            break;
        }
        if (p.trimZeros) printer(os, p, num, TrimZerosTag<true> {});
        else printer(os, p, num, TrimZerosTag<false> {});
    }

    template <class T>
    using enable_if_float = typename std::enable_if<std::is_floating_point<T>::value>::type;
    template <class T>
    using enable_if_not_float = typename std::enable_if<!std::is_floating_point<T>::value>::type;

    template <class CharT, class Traits>
    class prprint_proxy {
    public:
        using ostream_type = std::basic_ostream<CharT, Traits>;

        prprint_proxy(ostream_type& os, PrPrint p)
            : os_(os),
              p_(p) {}

        template <class Rhs,
                  class = enable_if_not_float<Rhs>>
        prprint_proxy& operator<<(const Rhs& rhs) {
            os_ << rhs;
            return *this;
        }

        prprint_proxy& operator<<(PrPrint rhs) {
            p_ = rhs;
            return *this;
        }

        template <class T,
                  class = enable_if_float<T>>
        prprint_proxy& operator<<(T num) {
            printer(os_, p_, num);
            return *this;
        }

    private:
        ostream_type& os_;
        PrPrint p_;
    };

} // namespace detail

template <class CharT, class Traits, class T,
          class = detail::enable_if_float<T>>
inline void print(std::basic_ostream<CharT, Traits>& os, PrPrint p, T num) {
    detail::printer(os, p, num);
}

template <class CharT, class Traits>
inline detail::prprint_proxy<CharT, Traits>
operator<<(std::basic_ostream<CharT, Traits>& os, PrPrint p) {
    return detail::prprint_proxy<CharT, Traits>(os, p);
}

} // namespace prprint

#endif // PRECISION_PRINT_HPP_INCLUDE