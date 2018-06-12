/* ----------------------------------------------------------------------------
Copyright (c) 2016,2017, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the Apache License, Version 2.0. A copy of the License can be
found in the file "license.txt" at the root of this distribution.
-----------------------------------------------------------------------------*/

#ifdef __cplusplus
#include <exception>
#include <utility>
#endif

#include "libhandler.h"
#include <malloc.h>
#include <string.h>
#include <errno.h>


lh_exception lh_exn_nomem = {
  ENOMEM, "Out of memory", NULL, 0
};

void lh_throw_nomem() {
  lh_throw(&lh_exn_nomem);
}

void lh_exception_free(lh_exception* exn) {
  if (exn == NULL) return;
  if ((exn->_is_alloced & 0x04) && exn->data != NULL) free(exn->data);
  if ((exn->_is_alloced & 0x02) && exn->msg != NULL) free((void*)(exn->msg));
  if ((exn->_is_alloced & 0x01)) free(exn);
}

lh_exception* lh_exception_alloc_ex(int code, const char* msg, void* data, int _is_alloced) {
  lh_exception* exn = (lh_exception*)malloc(sizeof(lh_exception));
  if (exn == NULL) return &lh_exn_nomem;
  exn->code = code;
  exn->msg = msg;
  exn->data = data;
  exn->_is_alloced = _is_alloced | 0x01;
  return exn;
}

lh_exception* lh_exception_alloc_strdup(int code, const char* msg) {
  return lh_exception_alloc_ex(code, _strdup(msg), NULL, 0x02);
}

lh_exception* lh_exception_alloc(int code, const char* msg) {
  return lh_exception_alloc_ex(code, msg, NULL, 0);
}

/*-----------------------------------------------------------------
Define operations
-----------------------------------------------------------------*/
LH_DEFINE_EFFECT1(exn, _throw)

void lh_throw(const lh_exception* e) { 
  lh_yield(LH_OPTAG(exn,_throw), lh_value_ptr(e)); 
}

void lh_throw_errno(int eno) {
  char msg[256];
  strerror_s(msg, 255, eno); msg[255] = 0;
  lh_throw(lh_exception_alloc_strdup(eno, msg));
}


static lh_value _handle_exn_throw(lh_resume r, lh_value local, lh_value arg) {
  lh_exception** exn = (lh_exception**)(lh_ptr_value(local));
  if (exn != NULL) *exn = (lh_exception*)lh_ptr_value(arg);
  return lh_value_null;
}

static const lh_operation _exn_ops[] = {
  { LH_OP_NORESUME, LH_OPTAG(exn,_throw), &_handle_exn_throw },
  { LH_OP_NULL, lh_op_null, NULL }
};
static const lh_handlerdef exn_def = { LH_EFFECT(exn), NULL, NULL, NULL, _exn_ops };

// Convert an exceptional computation to an exceptional value
static lh_value exn_try(lh_exception** exn, lh_value(*action)(lh_value), lh_value arg ) {
  *exn = NULL;
  return lh_handle(&exn_def, lh_value_any_ptr(exn), action, arg);
}


// Convert an exceptional computation to an exceptional value
lh_value lh_try(lh_exception** exn, lh_actionfun* action, lh_value arg) {
  #ifdef __cplusplus
  try {
  #endif
    return exn_try(exn, action, arg);
  #ifdef __cplusplus
  } 
  catch (std::exception e) {
    *exn = lh_exception_alloc_strdup(EOTHER, e.what());  // TODO: how to store a first class exception?
  }
  catch (...) {
    *exn = lh_exception_alloc(EOTHER, "Unknow error");
  }
  return lh_value_null;
  #endif
}
