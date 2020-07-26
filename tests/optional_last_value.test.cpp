#include <iterator>
#include <vector>

#include <signals/optional_last_value.hpp>

#include <catch2/catch.hpp>

using Result_t = sig::Optional_last_value<int>::Result_type;

TEST_CASE("Dereference each element and return last", "[optional_last_value]")
{
    auto vec        = std::vector<int>{1, 2, 3, 4, 5};
    auto olv        = sig::Optional_last_value<int>{};
    Result_t result = olv(std::begin(vec), std::end(vec));
    CHECK(bool(result));
    CHECK(*result == 5);
}

TEST_CASE("Return std::nullopt for empty range", "[optional_last_value]")
{
    auto vec        = std::vector<int>{};
    auto olv        = sig::Optional_last_value<int>{};
    Result_t result = olv(std::begin(vec), std::end(vec));
    CHECK_FALSE(bool(result));
}
