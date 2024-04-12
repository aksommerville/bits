#include "opt/serial/sr_primitives.c"
#include "test/test.h"
#include <math.h>

/* sr_memcasecmp
 */
 
static int test_sr_memcasecmp() {
  ASSERT_NOT(sr_memcasecmp(" abcdefghijklmnopqrstuvwxyz 0123456789 "," ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 ",39))
  ASSERT_NOT(sr_memcasecmp(" ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 "," abcdefghijklmnopqrstuvwxyz 0123456789 ",39))
  ASSERT(sr_memcasecmp("  "," \t",2))
  return 0;
}

/* Integer eval and repr.
 */
 
static int test_int_eval_repr() {
  int v;

  ASSERT_FAILURE(sr_int_eval(&v,"",0))
  ASSERT_FAILURE(sr_int_eval(&v,"-",1))
  ASSERT_FAILURE(sr_int_eval(&v,"+",1))
  ASSERT_FAILURE(sr_int_eval(&v,"a",1))
  ASSERT_FAILURE(sr_int_eval(&v," 1",2))
  
  ASSERT_INTS(sr_int_eval(&v,"0",1),2)
  ASSERT_INTS(v,0)
  ASSERT_INTS(sr_int_eval(&v,"-0",2),2)
  ASSERT_INTS(v,0)
  ASSERT_INTS(sr_int_eval(&v,"+0",2),2)
  ASSERT_INTS(v,0)
  ASSERT_INTS(sr_int_eval(&v,"10",2),2)
  ASSERT_INTS(v,10)
  ASSERT_INTS(sr_int_eval(&v,"0b10",-1),2)
  ASSERT_INTS(v,2)
  ASSERT_INTS(sr_int_eval(&v,"0o10",-1),2)
  ASSERT_INTS(v,8)
  ASSERT_INTS(sr_int_eval(&v,"0d10",-1),2)
  ASSERT_INTS(v,10)
  ASSERT_INTS(sr_int_eval(&v,"0x10",-1),2)
  ASSERT_INTS(v,16)
  ASSERT_INTS(sr_int_eval(&v,"-0b10",-1),2)
  ASSERT_INTS(v,-2)
  ASSERT_INTS(sr_int_eval(&v,"-0o10",-1),2)
  ASSERT_INTS(v,-8)
  ASSERT_INTS(sr_int_eval(&v,"-0d10",-1),2)
  ASSERT_INTS(v,-10)
  ASSERT_INTS(sr_int_eval(&v,"-0x10",-1),2)
  ASSERT_INTS(v,-16)
  ASSERT_INTS(sr_int_eval(&v,"0B10",-1),2)
  ASSERT_INTS(v,2)
  ASSERT_INTS(sr_int_eval(&v,"0O10",-1),2)
  ASSERT_INTS(v,8)
  ASSERT_INTS(sr_int_eval(&v,"0D10",-1),2)
  ASSERT_INTS(v,10)
  ASSERT_INTS(sr_int_eval(&v,"0X10",-1),2)
  ASSERT_INTS(v,16)
  ASSERT_INTS(sr_int_eval(&v,"0xABCDEF",8),2)
  ASSERT_INTS(v,0xabcdef)
  ASSERT_INTS(sr_int_eval(&v,"0xfedcba",8),2)
  ASSERT_INTS(v,0xfedcba)
  
  ASSERT_FAILURE(sr_int_eval(&v,"0b2",-1))
  ASSERT_FAILURE(sr_int_eval(&v,"0o8",-1))
  ASSERT_FAILURE(sr_int_eval(&v,"0da",-1))
  ASSERT_FAILURE(sr_int_eval(&v,"0xg",-1))
  ASSERT_INTS(sr_int_eval(&v,"0b1",-1),2)
  ASSERT_INTS(v,1)
  ASSERT_INTS(sr_int_eval(&v,"0o7",-1),2)
  ASSERT_INTS(v,7)
  ASSERT_INTS(sr_int_eval(&v,"0d9",-1),2)
  ASSERT_INTS(v,9)
  ASSERT_INTS(sr_int_eval(&v,"0xf",-1),2)
  ASSERT_INTS(v,15)
  
  ASSERT_INTS(sr_int_eval(&v,"2147483647",-1),2)
  ASSERT_INTS(v,2147483647)
  ASSERT_INTS(sr_int_eval(&v,"2147483648",-1),1)
  ASSERT_INTS(v,-2147483648)
  ASSERT_INTS(sr_int_eval(&v,"0b1111111111111111111111111111111",33),2)
  ASSERT_INTS(v,2147483647)
  ASSERT_INTS(sr_int_eval(&v,"0b10000000000000000000000000000000",34),1)
  ASSERT_INTS(v,-2147483648)
  ASSERT_INTS(sr_int_eval(&v,"0x7fffffff",-1),2)
  ASSERT_INTS(v,2147483647)
  ASSERT_INTS(sr_int_eval(&v,"0x80000000",-1),1)
  ASSERT_INTS(v,-2147483648)
  ASSERT_INTS(sr_int_eval(&v,"4294967295",-1),1)
  ASSERT_INTS(v,4294967295)
  ASSERT_INTS(sr_int_eval(&v,"4294967296",-1),0)
  ASSERT_INTS(v,0)
  ASSERT_INTS(sr_int_eval(&v,"0b11111111111111111111111111111111",34),1)
  ASSERT_INTS(v,4294967295)
  ASSERT_INTS(sr_int_eval(&v,"0b100000000000000000000000000000000",35),0)
  ASSERT_INTS(v,0)
  ASSERT_INTS(sr_int_eval(&v,"0xffffffff",-1),1)
  ASSERT_INTS(v,4294967295)
  ASSERT_INTS(sr_int_eval(&v,"0x100000000",-1),0)
  ASSERT_INTS(v,0)
  
  char text[128];
  int textc;
  
  #define ASSERT_REPR(srcv,decsint,decuint,hexuint) { \
    ASSERT_INTS(textc=sr_decsint_repr(text,sizeof(text),srcv),sizeof(decsint)-1) \
    ASSERT_STRINGS(text,textc,decsint,sizeof(decsint)-1) \
    ASSERT_INTS(textc=sr_decuint_repr(text,sizeof(text),srcv,0),sizeof(decuint)-1) \
    ASSERT_STRINGS(text,textc,decuint,sizeof(decuint)-1) \
    ASSERT_INTS(textc=sr_hexuint_repr(text,sizeof(text),srcv,0,0),sizeof(hexuint)-1) \
    ASSERT_STRINGS(text,textc,hexuint,sizeof(hexuint)-1) \
  }
  ASSERT_REPR(0,"0","0","0")
  ASSERT_REPR(1,"1","1","1")
  ASSERT_REPR(-1,"-1","4294967295","ffffffff")
  ASSERT_REPR(9,"9","9","9")
  ASSERT_REPR(10,"10","10","a")
  ASSERT_REPR(15,"15","15","f")
  ASSERT_REPR(16,"16","16","10")
  ASSERT_REPR(-9,"-9","4294967287","fffffff7")
  ASSERT_REPR(-10,"-10","4294967286","fffffff6")
  ASSERT_REPR(-2147483648,"-2147483648","2147483648","80000000")
  #undef ASSERT_REPR
  
  ASSERT_INTS(textc=sr_decuint_repr(text,sizeof(text),12345,8),8)
  ASSERT_STRINGS(text,textc,"00012345",8)
  ASSERT_INTS(textc=sr_hexuint_repr(text,sizeof(text),0xbeef,1,0),6)
  ASSERT_STRINGS(text,textc,"0xbeef",6)
  ASSERT_INTS(textc=sr_hexuint_repr(text,sizeof(text),0xbeef,1,5),7)
  ASSERT_STRINGS(text,textc,"0x0beef",7)

  return 0;
}

/* sr_double_eval and sr_double_repr
 */
 
static int test_float_eval_repr() {
  char text[128];
  int textc;
  double v;
  
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),NAN))
  ASSERT_STRINGS(text,textc,"NAN",3)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),INFINITY))
  ASSERT_STRINGS(text,textc,"INF",3)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-INFINITY))
  ASSERT_STRINGS(text,textc,"-INF",4)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.0))
  ASSERT_STRINGS(text,textc,"0.0",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.1))
  ASSERT_STRINGS(text,textc,"0.1",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.2))
  ASSERT_STRINGS(text,textc,"0.2",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.3))
  ASSERT_STRINGS(text,textc,"0.3",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.4))
  ASSERT_STRINGS(text,textc,"0.4",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.5))
  ASSERT_STRINGS(text,textc,"0.5",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.6))
  ASSERT_STRINGS(text,textc,"0.6",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.7))
  ASSERT_STRINGS(text,textc,"0.7",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.8))
  ASSERT_STRINGS(text,textc,"0.8",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),0.9))
  ASSERT_STRINGS(text,textc,"0.9",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),1.0))
  ASSERT_STRINGS(text,textc,"1.0",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.1))
  ASSERT_STRINGS(text,textc,"-0.1",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.2))
  ASSERT_STRINGS(text,textc,"-0.2",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.3))
  ASSERT_STRINGS(text,textc,"-0.3",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.4))
  ASSERT_STRINGS(text,textc,"-0.4",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.5))
  ASSERT_STRINGS(text,textc,"-0.5",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.6))
  ASSERT_STRINGS(text,textc,"-0.6",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.7))
  ASSERT_STRINGS(text,textc,"-0.7",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.8))
  ASSERT_STRINGS(text,textc,"-0.8",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-0.9))
  ASSERT_STRINGS(text,textc,"-0.9",-1)
  ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),-1.0))
  ASSERT_STRINGS(text,textc,"-1.0",-1)
  
  #define REPREVAL(start) { \
    ASSERT_CALL(textc=sr_double_repr(text,sizeof(text),start)) \
    ASSERT_INTS_OP(textc,<,sizeof(text)) \
    ASSERT_CALL(sr_double_eval(&v,text,textc),"text='%.*s'",textc,text) \
    ASSERT_FLOATS(start,v,0.001,"text='%.*s'",textc,text) \
  }
  REPREVAL(0.0)
  REPREVAL(-0.5)
  REPREVAL(-1.0)
  REPREVAL(0.5)
  REPREVAL(1.0)
  REPREVAL(123.456)
  REPREVAL(-123.456)
  REPREVAL(12.34e5)
  REPREVAL(-12.34e5)
  REPREVAL(12.34e-5)
  REPREVAL(-12.34e-5)
  #undef REPREVAL

  return 0;
}

/* sr_measure_number, sr_string_measure.
 */
 
static int test_token_measurement() {
  ASSERT_INTS(sr_number_measure("",0,0),0)
  ASSERT_INTS(sr_number_measure(" 123",4,0),0)
  ASSERT_INTS(sr_number_measure("123 ",4,0),3)
  ASSERT_INTS(sr_number_measure("0xabcdefabcdef0123456789 ",25,0),24)
  ASSERT_INTS(sr_number_measure("0xabcdefabcdef0123456789.",25,0),24)
  ASSERT_INTS(sr_number_measure("123.toString()",14,0),3)
  ASSERT_INTS(sr_number_measure("+1 ",3,0),2)
  ASSERT_INTS(sr_number_measure("-1 ",3,0),2)
  ASSERT_INTS(sr_number_measure("+ ",2,0),0)
  ASSERT_INTS(sr_number_measure("123.456e7 ",10,0),9)
  ASSERT_INTS(sr_number_measure("123.456e+7 ",11,0),10)
  ASSERT_INTS(sr_number_measure("123.456e-7 ",11,0),10)
  ASSERT_INTS(sr_number_measure("123.4k6e+7 ",11,0),0)
  
  ASSERT_INTS(sr_string_measure("'abc' ",6,0),5)
  ASSERT_INTS(sr_string_measure("'abc\" ",6,0),0)
  ASSERT_INTS(sr_string_measure("\"a\" ",4,0),3)
  ASSERT_INTS(sr_string_measure("'a' ",4,0),3)
  ASSERT_INTS(sr_string_measure("`a` ",4,0),3)
  ASSERT_INTS(sr_string_measure("a ",2,0),0)
  ASSERT_INTS(sr_string_measure("'abc\\'xyz' ",11,0),10)
  return 0;
}

/* TOC
 */
 
int main(int argc,char **argv) {
  UTEST(test_sr_memcasecmp,serial)
  UTEST(test_int_eval_repr,serial)
  UTEST(test_float_eval_repr,serial)
  UTEST(test_token_measurement,serial)
  return 0;
}
