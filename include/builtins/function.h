#include "../object.h"
#include "../env.h"

typedef struct {
    NativeFn func;    
    char* name;
} NativeFunction;

