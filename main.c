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

    // Enable user to give an extra parameter "utf8bom" to output the three
    // byte sequence that tells that the data is encoded as UTF-8.
    // This is required by some programs (like notepad) to correctly
    // render UTF-8 characters. This is useful when the output of this
    // program is redirected to a file. With this parameter given, the file
    // will also contain the UTF-8 BOM.
    if (argc >= 3 && strcmp(args[2], "utf8bom"))
        printf("%c%c%c", 0xEF, 0xBB, 0xBF);

    // Open .class file given as parameter
    JavaClassFile jcf;
    openClassFile(&jcf, args[1]);

    // If everything went fine, print its content
    if (jcf.status == STATUS_OK)
        printClassFileInfo(&jcf);
    // Otherwise, show what went wrong
    else
        printClassFileDebugInfo(&jcf);

    closeClassFile(&jcf);
    return 0;
}
