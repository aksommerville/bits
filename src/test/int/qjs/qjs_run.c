#if USE_qjs

#include "test/test.h"
#include "opt/qjs/qjs.h"
#include "opt/fs/fs.h"
#include "quickjs.h"

/* Some functions to expose to our scripts.
 */
 
static JSValue multiply(JSContext *ctx,JSValueConst this,int argc,JSValueConst *argv) {
  if (argc!=2) return JS_ThrowTypeError(ctx,"multiply takes 2 arguments");
  int32_t a=0,b=0;
  JS_ToInt32(ctx,&a,argv[0]);
  JS_ToInt32(ctx,&b,argv[1]);
  return JS_NewInt32(ctx,a*b);
}

static const JSCFunctionListEntry exports[]={
  JS_CFUNC_DEF("multiply",0,multiply),
};

/* Load a Javascript module and run it.
 */
 
ITEST(qjs_basic_run) {
  const char *jspath="src/js/hello.js";
  void *serial=0;
  int serialc=file_read(&serial,jspath);
  ASSERT_CALL(serialc,"%s: Failed to read file",jspath)
  
  struct qjs *qjs=qjs_new();
  ASSERT(qjs)
  ASSERT_CALL(qjs_set_exports(qjs,"mymodule",exports,sizeof(exports)/sizeof(exports[0])))
  
  ASSERT_CALL(qjs_add_module(qjs,1,serial,serialc,jspath))
  free(serial);
  
  ASSERT_CALL(qjs_link_function(qjs,1,1,"main"))
  
  int result=0;
  ASSERT_CALL(result=qjs_callf(qjs,1,1,"iis",4,3,"seven"))
  ASSERT_INTS(result,12)
  
  qjs_del(qjs);
  return 0;
}

#else
int qjs_basic_run() { return 0; }
#endif
