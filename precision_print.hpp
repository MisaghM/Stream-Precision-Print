#include <ostream>
#include <type_traits>

namespace prprint {

enum class Rounding {
    keep,
    up,
    down
};

struct PrPrint {
    explicit PrPrint(unsigned precision, bool trimZeros = true, Rounding mode = Rounding::keep)
        : precision_(precision),
          trimZeros_(trimZeros),
          mode_(mode) {}

    unsigned precision_;
    bool trimZeros_;
    Rounding mode_;
};

namespace detail {

    template <class T>
    using enable_if_float = typename std::enable_if<std::is_floating_point<T>::value>::type;
    template <class T>
    using enable_if_not_float = typename std::enable_if<!std::is_floating_point<T>::value>::type;

    template <class Char, class CharT, class T>
    inline void printer(std::basic_ostream<Char, CharT>& os, PrPrint p, T num) {
        os << num;
    }

    template <class Char, class CharT>
    class prprint_proxy {
    public:
        using OSType = std::basic_ostream<Char, CharT>;

        prprint_proxy(OSType& os, PrPrint p)
            : os_(os),
              p_(p) {}

        template <class Rhs,
                  class = enable_if_not_float<Rhs>>
        prprint_proxy& operator<<(const Rhs& rhs) {
            os_ << rhs;
            return *this;
        }

        template <class T,
                  class = enable_if_float<T>>
        prprint_proxy& operator<<(T num) {
            printer(os_, p_, num);
            return *this;
        }

    private:
        OSType& os_;
        PrPrint p_;
    };

} // namespace detail

template <class Char, class CharT, class T,
          class = detail::enable_if_float<T>>
inline void PrecisionPrint(std::basic_ostream<Char, CharT>& os, PrPrint p, T num) {
    detail::printer(os, p, num);
}

template <class Char, class CharT>
inline detail::prprint_proxy<Char, CharT>
operator<<(std::basic_ostream<Char, CharT>& os, PrPrint p) {
    return detail::prprint_proxy<Char, CharT>(os, p);
}

} // namespace prprint