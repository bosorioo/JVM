#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "javaclassfile.h"

int main(int argc, char* args[])
{
    if (argc <= 1)
    {
        printf("First parameter should be the path to a .class file.\n");
        return 0;
    }

    JavaClassFile jcf;

    openClassFile(&jcf, args[1]);

    if (jcf.status == STATUS_OK)
        printClassFileInfo(&jcf);
    else
        printClassFileDebugInfo(&jcf);

    closeClassFile(&jcf);

    return 0;
}
