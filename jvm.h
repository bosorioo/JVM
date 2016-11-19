#ifndef JVM_H
#define JVM_H

typedef struct JavaVirtualMachine JavaVirtualMachine;

#include <stdint.h>
#include "methodarea.h"
#include "javaclass.h"
#include "framestack.h"

struct JavaVirtualMachine
{
    method_area ma;
    FrameStack* frames;
};

void initJVM(JavaVirtualMachine* jvm, JavaClass* mainClass);

#endif // JVM_H
