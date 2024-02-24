#include "test/test.h"
#include <stdlib.h>

static int a_thing_that_works() {
  ASSERT_INTS(1,1,"one is one")
  ASSERT_INTS_OP(1,!=,2,"one is not two")
  return 0;
}

static int a_thing_that_doesnt_work() {
  ASSERT(0,"zero is true")
  return 0;
}

int main(int argc,char **argv) {
  UTEST(a_thing_that_works,abc def ghi)
  XXX_UTEST(a_thing_that_doesnt_work,ghi xyz)
  return 0;
}
