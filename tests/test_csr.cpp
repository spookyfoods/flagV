#include "csr.h"
#include <catch2/catch_test_macros.hpp>

CsrFile csr;

TEST_CASE("CSR Access") {
    SECTION("mhartid") {
        REQUIRE(csr.isAccessValid(0xF14, PrivLvl::U, true) == false);
        REQUIRE(csr.isAccessValid(0xF14, PrivLvl::U, false) == false);
        REQUIRE(csr.isAccessValid(0xF14, PrivLvl::S, true) == false);
        REQUIRE(csr.isAccessValid(0xF14, PrivLvl::S, false) == false);
        REQUIRE(csr.isAccessValid(0xF14, PrivLvl::M, true) == true);
        REQUIRE(csr.isAccessValid(0xF14, PrivLvl::M, false) == true);
    }
}

// TEST_CASE("Writes Persist") {

//     SECTION("Writing to CSRs and looping back") {

//     }
// }
