#include "test/test.h"
#include "wamr/wamr.h"
#include "fs/fs.h"
#include "wasm_export.h"

/* Some functions to expose to our Wasm executables.
 */
 
static int multiply(wasm_exec_env_t ee,int a,int b) {
  return a*b;
}
 
static NativeSymbol exports[]={
  {"multiply",multiply,"(ii)i"},
};

/* Load a WebAssembly module and run it.
 */
 
ITEST(wamr_basic_run) {
  const char *wpath="out/wasm/hello.wasm";
  void *serial=0;
  int serialc=file_read(&serial,wpath);
  ASSERT_CALL(serialc,"%s: Failed to read file",wpath)
  //fprintf(stderr,"%s: Read %d bytes.\n",wpath,serialc);
  
  struct wamr *wamr=wamr_new();
  ASSERT(wamr)
  ASSERT_CALL(wamr_set_exports(wamr,exports,sizeof(exports)/sizeof(exports[0])))
  
  ASSERT_CALL(wamr_add_module(wamr,1,serial,serialc,wpath))
  free(serial);
  
  ASSERT_CALL(wamr_link_function(wamr,1,1,"cant_call_it_main"))
  
  uint32_t argv[1]={4};
  ASSERT_CALL(wamr_call(wamr,1,1,argv,1))
  //fprintf(stderr,"Return value from %s:cant_call_it_main: %d\n",wpath,argv[0]);
  ASSERT_INTS(argv[0],12)
  
  wamr_del(wamr);
  return 0;
}
