#include "src/tint/utils/strconv/parse_num.h"

#include "check.h"
#include <cmath>
#include <cstdio>
#include <string_view>

int main() {
    using namespace tint::strconv;
    CHECK(ParseFloat("1.25") == 1.25f);
    CHECK(ParseDouble("-2.5e2") == -250.0);
    CHECK(ParseFloat("1e100") == ParseNumberError::kResultOutOfRange);
    CHECK(ParseFloat("1e-100") == ParseNumberError::kResultOutOfRange);
    CHECK(ParseFloat("1e-40") == tint::Success);
    CHECK(ParseDouble("1e-300") == tint::Success);
    CHECK(ParseDouble("0e-999") == 0.0);
    CHECK(ParseFloat("") == ParseNumberError::kUnparsable);
    CHECK(ParseFloat(" 1") == ParseNumberError::kUnparsable);
    CHECK(ParseFloat("\t1") == ParseNumberError::kUnparsable);
    CHECK(ParseFloat("+1") == ParseNumberError::kUnparsable);
    CHECK(ParseFloat("1x") == ParseNumberError::kUnparsable);
    CHECK(ParseFloat("0x1p2") == ParseNumberError::kUnparsable);
    CHECK(ParseFloat("-0x1p2") == ParseNumberError::kUnparsable);
    CHECK(ParseFloat(std::string_view("1.25suffix", 4)) == 1.25f);
    CHECK(ParseFloat(std::string_view("1\0x", 3)) == ParseNumberError::kUnparsable);
    CHECK(std::isinf(ParseFloat("inf").Get()));
    CHECK(std::isnan(ParseDouble("nan").Get()));
    CHECK(ParseInt("42") == 42);
    CHECK(ParseUint("-1") == ParseNumberError::kUnparsable);
    std::puts("PASS: 20 Tint number parsing checks");
}
