#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "javaclassfile.h"

int main(int argc, char* args[])
{
    if (argc <= 1)
    {
        printf("First parameter should be the path to a .class file.\n");
        return 0;
    }

    if (argc >= 3 && strcmp(args[2], "utf8bom"))
        printf("%c%c%c", 0xEF, 0xBB, 0xBF);

    JavaClassFile jcf;

    openClassFile(&jcf, args[1]);

    if (jcf.status == STATUS_OK)
        printClassFileInfo(&jcf);
    else
        printClassFileDebugInfo(&jcf);

    closeClassFile(&jcf);

    return 0;
}
