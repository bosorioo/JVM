#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "javaclassfile.h"

int main()
{
    const char* path = "C:/Users/Bruno/Documents/NetBeansProjects/Sketch/src/sketch/Sketch.class";

    JavaClassFile jcf;

    openClassFile(&jcf, path);

    if (jcf.status == STATUS_OK)
        printClassFileInfo(&jcf);
    else
        printClassFileDebugInfo(&jcf);

    closeClassFile(&jcf);

    return 0;
}
