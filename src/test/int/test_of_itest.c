#include "test/test.h"

ITEST(no_tags) {
  ASSERT(1==1)
  return 0;
}

ITEST(run_this_always,abc def ghi) {
  ASSERT_NOT(1==2)
  return 0;
}

XXX_ITEST(dont_run_broke_somehow,abc xyz) {
  ASSERT(3==4,"This assertion fails but i cant figure out why")
  return 0;
}
