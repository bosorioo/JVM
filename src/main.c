#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "javaclass.h"
#include "jvm.h"
#include "memoryinspect.h"

int main(int argc, char* args[])
{
    if (argc <= 1)
    {
        printf("First parameter should be the path to a .class file.\n");
        printf("Following parameters could be:\n");
        printf(" -c \t Shows the content of the .class file\n");
        printf(" -e \t Execute the method 'main' from the class\n");
        printf(" -b \t Adds UTF-8 BOM to the output\n");
        printf(" -r \t Disables the simulated java/lang/String and java/lang/System classes\n");
        return 0;
    }

    uint8_t printClassContent = 0;
    uint8_t executeClassMain = 0;
    uint8_t includeBOM = 0;
    uint8_t simulateStringAndSystemClasses = 1;

    int argIndex;

    for (argIndex = 2; argIndex < argc; argIndex++)
    {
        if (!strcmp(args[argIndex], "-c"))
            printClassContent = 1;
        else if (!strcmp(args[argIndex], "-e"))
            executeClassMain = 1;
        else if (!strcmp(args[argIndex], "-b"))
            includeBOM = 1;
        else if (!strcmp(args[argIndex], "-r"))
            simulateStringAndSystemClasses = 0;
        else
            printf("Unknown argument #%d ('%s')\n", argIndex, args[argIndex]);
    }

    if (printClassContent)
    {
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
        if (jc.status != CLASS_STATUS_OK)
            printClassFileDebugInfo(&jc);
        // Otherwise, is requested by the user, print the class content
        else if (printClassContent)
            printClassFileInfo(&jc);

        closeClassFile(&jc);
    }

    if (executeClassMain)
    {
        JavaVirtualMachine jvm;
        initJVM(&jvm);
        jvm.simulatingSystemAndStringClasses = simulateStringAndSystemClasses;

        size_t inputLength = strlen(args[1]);

        // This is to remove the ".class" from the file name. Example:
        //   "C:/mypath/file.class" becomes "C:/mypath/file"
        // That is done because the resolveClass function adds the .class
        // when looking for files to open. This is the best behavior because
        // other classes paths from within the class file don't have the
        // .class file extension.
        if (inputLength > 6 && strcmp(args[1] + inputLength - 6, ".class") == 0)
        {
            inputLength -= 6;
            args[1][inputLength] = '\0';
        }

        LoadedClasses* mainLoadedClass;

        if (resolveClass(&jvm, (const uint8_t*)args[1], inputLength, &mainLoadedClass))
            executeJVM(&jvm, mainLoadedClass->jc);

#ifdef DEBUG
        printf("Execution finished. Status: %d\n", jvm.status);
#endif // DEBUG

        deinitJVM(&jvm);
    }

    return 0;
}
