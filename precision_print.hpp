#ifndef PRECISION_PRINT_HPP_INCLUDE
#define PRECISION_PRINT_HPP_INCLUDE

#include <cmath>
#include <cstdio>
#include <cwchar>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

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

    template <class CharT, class Traits, class T>
    inline void printer(std::basic_ostream<CharT, Traits>& os, PrPrint p, T num, TrimZerosTag<false>) {
        const auto prevPrecision = os.precision(p.precision);
        const auto prevFmtflags = os.setf(std::ios_base::fixed, std::ios_base::floatfield);
        os.unsetf(std::ios_base::adjustfield);
        os << num;
        os.flags(prevFmtflags);
        os.precision(prevPrecision);
    }

    template <class CharT, class Traits, class T>
    inline void printer(std::basic_ostream<CharT, Traits>& os, PrPrint p, T num, TrimZerosTag<true>) {
        const auto osFlags = os.flags();
        std::basic_ostringstream<CharT, Traits> sstr;
        sstr.precision(p.precision);
        sstr.flags(std::ios_base::fixed | (osFlags & (std::ios_base::showpos | std::ios_base::showpoint)));

        sstr << num;
        std::basic_string<CharT, Traits> s(sstr.str());

        if (p.precision != 0u) {
            s.erase(s.find_last_not_of(static_cast<CharT>('0')) + 1, std::basic_string<CharT, Traits>::npos);
            if (s.back() == static_cast<CharT>('.') && !(osFlags & std::ios_base::showpoint)) s.pop_back();
        }
        os << s;
    }

    template <class T>
    inline void printer(std::ostream& os, PrPrint p, T num, TrimZerosTag<true>) {
        const auto osFlags = os.flags();
        const auto showPoint = osFlags & std::ios_base::showpoint;

        const char* format = (osFlags & std::ios_base::showpos) ? "%+.*f" : "%.*f";
        const int len = std::snprintf(nullptr, 0, format, p.precision, num);
        std::string s(len + 1, 0);
        std::snprintf(&s[0], len + 1, format, p.precision, num);
        s.pop_back();

        if (p.precision != 0u) {
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.' && !showPoint) s.pop_back();
        }
        else if (showPoint) s.push_back('.');
        os << s;
    }

    template <class T>
    inline void printer(std::wostream& os, PrPrint p, T num, TrimZerosTag<true>) {
        const auto osFlags = os.flags();
        const auto showPoint = osFlags & std::ios_base::showpoint;

        const char* format;
        const wchar_t* wformat;
        if (osFlags & std::ios_base::showpos) {
            format = "%+.*f";
            wformat = L"%+.*f";
        }
        else {
            format = "%.*f";
            wformat = L"%.*f";
        }

        const int len = std::snprintf(nullptr, 0, format, p.precision, num);
        std::wstring s(len + 1, 0);
        std::swprintf(&s[0], len + 1, wformat, p.precision, num);
        s.pop_back();

        if (p.precision != 0u) {
            s.erase(s.find_last_not_of(L'0') + 1, std::wstring::npos);
            if (s.back() == L'.' && !showPoint) s.pop_back();
        }
        else if (showPoint) s.push_back(L'.');
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