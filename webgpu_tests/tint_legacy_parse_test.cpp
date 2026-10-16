#include "src/tint/utils/strconv/parse_num.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string_view>

int main() {
    using namespace tint::strconv;
    assert(ParseFloat("1.25") == 1.25f);
    assert(ParseDouble("-2.5e2") == -250.0);
    assert(ParseFloat("1e100") == ParseNumberError::kResultOutOfRange);
    assert(ParseFloat("1e-100") == ParseNumberError::kResultOutOfRange);
    assert(ParseFloat("1e-40") == tint::Success);
    assert(ParseDouble("1e-300") == tint::Success);
    assert(ParseDouble("0e-999") == 0.0);
    assert(ParseFloat("") == ParseNumberError::kUnparsable);
    assert(ParseFloat(" 1") == ParseNumberError::kUnparsable);
    assert(ParseFloat("\t1") == ParseNumberError::kUnparsable);
    assert(ParseFloat("+1") == ParseNumberError::kUnparsable);
    assert(ParseFloat("1x") == ParseNumberError::kUnparsable);
    assert(ParseFloat("0x1p2") == ParseNumberError::kUnparsable);
    assert(ParseFloat("-0x1p2") == ParseNumberError::kUnparsable);
    assert(ParseFloat(std::string_view("1.25suffix", 4)) == 1.25f);
    assert(ParseFloat(std::string_view("1\0x", 3)) == ParseNumberError::kUnparsable);
    assert(std::isinf(ParseFloat("inf").Get()));
    assert(std::isnan(ParseDouble("nan").Get()));
    assert(ParseInt("42") == 42);
    assert(ParseUint("-1") == ParseNumberError::kUnparsable);
    std::puts("PASS: 20 Tint number parsing checks");
}
