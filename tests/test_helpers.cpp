#include "common.h"

#include "helpers.h"

TEST(ParseParentPID_simple)
{
    char line[] = "1 (init) S 0 1 1 0 -1 4202752 253 667895 22 419 0 146 849 157 20 0 1 0 0 4063232 "
                  "132 18446744073709551615 1 1 0 0 0 0 0 1475401980 671819267 18446744073709551615 "
                  "0 0 0 3 0 0 42 0 0";
    CHECK_EQUAL(0, dbg_mi::ParseParentPID(line));
}

TEST(ParseParentPID_spaces)
{
    char line[] = "6966 (watches test) S 6960 6966 6966 0 -1 8192 700 0 0 0 0 0 0 0 20 0 1 0 33744903 "
                  "12070912 244 18446744073709551615 4194304 4220368 140737488346144 140737488344776 "
                  "140737340978176 0 0 16781312 0 0 0 0 17 2 0 0 0 0 0";
    CHECK_EQUAL(6960, dbg_mi::ParseParentPID(line));
}

TEST(ParseParentPID_parens)
{
    char line[] = "6966 (watches(testme) test) S 6961 6966 6966 0 -1 8192 700 0 0 0 0 0 0 0 20 0 1 0 33744903 "
                  "12070912 244 18446744073709551615 4194304 4220368 140737488346144 140737488344776 "
                  "140737340978176 0 0 16781312 0 0 0 0 17 2 0 0 0 0 0";
    CHECK_EQUAL(6961, dbg_mi::ParseParentPID(line));
}
