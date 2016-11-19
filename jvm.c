#include "jvm.h"

void initJVM(JavaVirtualMachine* jvm, JavaClass* jc)
{
    initMethodArea(&jvm->ma);
    addClassToMethodArea(&jvm->ma, jc);
}
