
# Stream Precision Print [![license](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://github.com/MisaghM/Stream-Precision-Print/blob/main/LICENSE "Repository License")

## About

A simple way to print floating-point types with a certain precision.  
Used like a stream manipulator.  
  
As simple as:

```cpp
double a = 12.3456;
std::cout << prprint::PrPrint(2) << a; // 12.34
```

*Requires at least C++11.*

## Features

With this header-only library, you can easily **trim trailing 0s** after setting the **precision**, and also change the **rounding** mode.  
  
(Keep in mind that the rounding methods are very simple, error-prone and should only be used for simple tasks)

## Example Usage

```cpp
#include <iostream>

#include "precision_print.hpp"

int main() {
    // All of the library is in the 'prprint' namespace
    // The printing options are kept in a PrPrint struct:
    // (Precision, TrimTrailingZeros=false, RoundingMode=keep)

    // To format, you can either use the 'print' function:
    // prprint::print(ostream, float, PrPrint);
    // or use the stream proxy:
    // ostream << PrPrint(p) << a << b;
    // once applied, it will continue for the entire chain,
    // and will print non float variables like normal.
    // when met with another PrPrint, it will change for the rest of the chain.

    using namespace prprint;

    double a = 4.200;
    std::cout << PrPrint(2) << a << '\n' // 4.20
              << PrPrint(2, true) << a; // 4.2

    float b = 2.38f;
    std::cout << PrPrint(1, false, Rounding::downward) << b; // 2.3

    return 0;
}
```

## PRPRINT_UNIFORM_PRINTER

To trim trailing zeros, the float must be printed to a string, trimmed, and then go to the output stream.  
A simple way to achieve this is templated for all character types and uses `ostringstream`.

But some overloads have been written for `char` and `wchar_t` types which are faster than the basic method.  
It uses `snprintf` or `to_chars` if C++17 is detected.  
(the `__cplusplus` macro is used so you will need the `/Zc:__cplusplus` flag when compiling with MSVC)

After some testing, it seems that with C++17, my overloads are faster on both GCC and MSVC.  
But without C++17, while still faster on MSVC, it did not have much difference on GCC.

So in case you want to disable them and only use the ostringstream method, you can define this macro:

```cpp
#define PRPRINT_UNIFORM_PRINTER
#include "precision_print.hpp"
```

*This library is not the most optimized way of printing a float.  
In fact, C++ streams are already pretty slow so if performance is your need, use an efficient formatting library such as [{fmt}](https://github.com/fmtlib/fmt/ "{fmt} GitHub")*
