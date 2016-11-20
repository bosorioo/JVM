#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "javaclass.h"
#include "jvm.h"

int main(int argc, char* args[])
{
    if (argc <= 1)
    {
        printf("First parameter should be the path to a .class file.\n");
        printf("Following parameters could be:\n");
        printf(" -c \t Shows the content of the .class file\n");
        printf(" -r \t Execute the method 'main' from the class\n");
        printf(" -b \t Adds UTF-8 BOM to the output\n");
        return 0;
    }

    uint8_t printClassContent = 0;
    uint8_t executeClassMain = 0;
    uint8_t includeBOM = 0;

    int argIndex;

    for (argIndex = 2; argIndex < argc; argIndex++)
    {
        if (strcmp(args[argIndex], "-c"))
            printClassContent = 1;
        else if (strcmp(args[argIndex], "-r"))
            executeClassMain = 1;
        else if (strcmp(args[argIndex], "-b"))
            includeBOM = 1;
        else
            printf("Unknown argument #%d ('%s')\n", argIndex, args[argIndex]);
    }

    // If requested, outputs the three byte sequence that tells that
    // the data is encoded as UTF-8.
    // This is required by some programs (like notepad) to correctly
    // render UTF-8 characters. This is useful when the output of this
    // program is redirected to a file. With this parameter given, the file
    // will also contain the UTF-8 BOM.
    if (includeBOM)
        printf("%c%c%c", 0xEF, 0xBB, 0xBF);

    // Open .class file given as parameter
    JavaClass jc;
    openClassFile(&jc, args[1]);

    // If something went wrong, show debug information
    if (jc.status != STATUS_OK)
        printClassFileDebugInfo(&jc);
    // Otherwise, is requested by the user, print the class content
    else if (printClassContent)
        printClassFileInfo(&jc);

    if (executeClassMain)
    {
        JavaVirtualMachine jvm;

        // TODO: It is not enough to open the class file and just run it,
        // it has to be resolved first. During resolution, other classes
        // could be loaded, like super classes or interfaces.
        addClassToMethodArea(&jvm, &jc);
        initJVM(&jvm);
    }



    closeClassFile(&jc);
    return 0;
}
