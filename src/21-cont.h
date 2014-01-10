// Copyright 2013 George King.
// Permission to use this file is granted in ploy/license.txt.

#include "20-env.h"


typedef struct _Step Step;
typedef Step (^Cont)(Obj);

struct _Step {
  Cont cont;
  Obj val;
};


#define Step0 (Step){ .cont=NULL, .val=VOID }

static Step step_mk(Cont cont, Obj val) {
  return (Step){.cont=cont, .val=val};
}

#define STEP(...) return step_mk(__VA_ARGS__)

