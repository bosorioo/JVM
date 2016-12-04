#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "javaclass.h"
#include "jvm.h"
#include "debugging.h"

int main(int argc, char* args[])
{
    if (argc <= 1)
    {
        printf("First parameter should be the path to a .class file.\n");
        printf("Following parameters could be:\n");
        printf(" -c \t Shows the content of the .class file\n");
        printf(" -e \t Execute the method 'main' from the class\n");
        printf(" -b \t Adds UTF-8 BOM to the output\n");
        return 0;
    }

    if (sizeof(int32_t*) > sizeof(int32_t))
    {
        printf("Warning: pointers cannot be stored in an int32_t.\n");
        printf("This is very likely to malfunction when handling references.\n");
        printf("Continue anyway? (Y/N)\n");

        int input;

        do {

            input = getchar();

            if (input == 'Y' || input == 'y')
                break;

            if (input == 'N' || input == 'n')
                exit(0);

            while (getchar() != '\n');

        } while (1);

    }

    uint8_t printClassContent = 0;
    uint8_t executeClassMain = 0;
    uint8_t includeBOM = 0;

    int argIndex;

    for (argIndex = 2; argIndex < argc; argIndex++)
    {
        if (!strcmp(args[argIndex], "-c"))
            printClassContent = 1;
        else if (!strcmp(args[argIndex], "-e"))
            executeClassMain = 1;
        else if (!strcmp(args[argIndex], "-b"))
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

    if (printClassContent)
    {
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

        setClassPath(&jvm, args[1]);

        if (resolveClass(&jvm, (const uint8_t*)args[1], inputLength, &mainLoadedClass))
            executeJVM(&jvm, mainLoadedClass);

#ifdef DEBUG
        printf("Execution finished. Status: %d\n", jvm.status);
#endif // DEBUG

        deinitJVM(&jvm);
    }

    return 0;
}

/// @mainpage Java Virtual Machine
///
/// <pre>
/// Group members:
///     Bruno Osorio
///     Joao Araujo
///     Leticia Ribeiro
///     Tiago Kfouri
///     Victoria Goularte
/// </pre>
///
///
///
/// @section intro Introduction
/// Simple implementation of java virtual machine that is able to read class files and
/// display their content and execute the class' main method.
/// <br>
/// Not all instructions have been implemented and there are other limitations.
/// Refer to section @ref limitations to see more.
/// <br>
///
///
///
/// @section stepguideJavaClass Step-by-step: display class content
/// -# Command line parameters are parsed in file main.c to get the class file path.
/// -# openClassFile() is called, defined in module javaclass.c.
/// -# file signature, version and constant pool count are read.
/// -# readConstantPoolEntry() is called several times to fill the constant pool.
/// -# checkConstantPoolValidity() will check if there are inconsistencies in the constant pool.
/// Refer to section @ref validity for verification mades and the ones that aren't.
/// -# this class index, super class index and access flags are read and checked using function checkClassIndexAndAccessFlags().
/// -# file name is checked against "this class" name to see if they match.
/// -# interface count and interfaces' index are read, checking if the indexes are a valid class index.
/// -# field count is read, and then several calls to readField() are made to fill in the fields
/// of the class. In this step, the class starts counting how many static fields and instance fields the
/// class has. Thus, when new instances of this class are created later, it is possible to know how many bytes
/// have to be allocated to hold the instance field data. When the class is initialized, it is also possible
/// to know the how many bytes it needs for its static field data.
/// -# method count is read, and then several calls to readMethod() are made to fill in the methods of the class.
/// -# attribute count is read, and then several calls to readAttribute() are made to fill in the attributes of the class.
/// -# a call to the function printClassFileInfo() will start printing general information of the class, like version, this
/// class index, super class index, constant pool/field/method/interface count and the class access flags.
/// -# printClassFileInfo() also make calls to printConstantPool(), printAllFields(), printMethods() and printAllAttributes().
///
/// Methods and fields can also have attributes, so their read functions also make calls to readAttributes.
/// If no errors occurred during the class loading procedure, then the function printClassFileInfo() is called, and
/// if there were errors, some debug information will show up telling what went wrong.
///
/// Note that if the class file name and the class actual name don't match (including package path), the class loading
/// will normally continue, but the JavaClass* structure holding the class information will be marked as name mismatch.
///
///
///
/// @section stepguideJVM Step-by-step: execute a class
/// -# Command line parameters are parsed in file main.c to get the class file path.
/// -# A JavaVirtualMachine variable needs to be initialized with a call to initJVM().
/// -# The file path is passed as parameter to resolveClass(), that will look for the class file, open it and load the
/// class information from it (refer to section @ref stepguideJavaClass to see how a class is read).
/// -# Once the class is loaded, it is added to the table of all loaded classes so far, see LoadedClasses, JavaVirtualMachine::classes.
/// -# A call to resolveClass() may trigger other classes resolution (super classes, interfaces implemented etc).
/// -# After the main class has been successfully resolved, a call to executeJVM() will initialize the class.
/// Class resolution won't initialize the class. The initialization is done
/// only when the class really needs to be initialized. Some instructions will trigger class initialization (new, putstatic, getstatic,
/// putfield, getfield, invokeinterface, invokespecial, invokestatic). executeJVM() will make the main class initialize. Class initialization
/// is done by reserving bytes to store the class static data (setting their initial value accordingly if a ConstantValue attribute is present),
/// and calling the <b><clinit></b> method of the class. See initClass().
/// -# After initializing the main class, a method with the signature <b>public static void main(String[] args)</b> will be searched and,
/// if found, called. Method calling is done via runMethod(). Note that the current implementation does not support command line parameter
/// passing to the main method.
/// -# When running a method, a frame for it will be created with a call to newFrame(). It will also be added to the stack of frames
/// (FrameStack) with a call to pushFrame(). A frame holds the class of the method, the bytecode of the method, the length in bytes of
/// the bytecode, the number of operands that need to be popped from this frame and pushed to a caller frame once the method returns,
/// a stack of operands (OperandStack) and an array of local variables.
/// -# Each instruction is fetched from the Code attribute of the method and the corresponding @ref InstructionFunction function pointer is
/// called. Fetch is done with a call to fetchOpcodeFunction(), which will return one of the functions defined in instructions.c.
/// -# Once a method is finished, its frame will be removed with a call to popFrame(). The frame will be deallocated with freeFrame().
/// If the method returns data, some of its operands in the OperandStack will be popped and pushed to the caller frame.
/// -# After all execution is done, a call to deinitJVM() will release all objects created and memory allocation associated with the
/// JVM.
///
/// All objects creation is done with calls from the following functions: newString(), newClassInstance(), newArray(), newObjectArray() and
/// newObjectMultiArray(). They are all released after the entire execution of the entry point class is over, during deinitialization of
/// the JVM. Specific object freeing is done with function deleteReference().
///
/// %String class has no methods implemented, it is only simulated. Class java/lang/System is specifically checked in some instruction for special
/// handling, like getting the static java/lang/System.out and calling its println method. This is implemented in file natives.c.
///
///
///
/// @section instructions Instructions Implementation
/// Each instruction is implemented entirely within its single C function. They are all
/// declared in file instructions.c.
/// <br>
/// All instructions functions share the prefix "instfunc_" and have the same
/// function prototype, which is:
/// @code
/// uint8_t instfunc_<instruction name>(JavaVirtualMachine* jvm, Frame* frame)
/// @endcode
/// A Frame structure contains all necessary information to run a method, i.e. the
/// method's bytecode along with its length, the method's program counter, an array
/// storing all local variables and a stack of operands (OperandStack).
/// The frame is used to manipulate the operands and the local variables, and read
/// instructions parameters from the method's bytecode. An OperandStack is manipulated
/// using functions pushOperand() and popOperand().
/// <br>
/// Instructions use the JavaVirtualMachine pointer to set errors (like insufficient
/// memory), resolve and look for classes, and create new objects
/// (arrays, class instances, strings). Object creation is done using functions
/// newString(), newClassInstance(), newArray(), newObjectArray() and newObjectMultiArray().
/// <br>
/// Some instructions are very similar to a few others. In those cases, instructions were
/// implemented using macros to reduce source code length. To see those macros, check file
/// intructions.c.
///
///
///
/// @section validity Validity and Errors
/// File validity.c contains various functions to help verify that some invalid configurations
/// of constant pool, fields and methods won't happen.
/// <br>
/// Right after reading all entries of the constant pool from the class file, function checkConstantPoolValidity()
/// is called. That function will iterate over all constant pool entries, checking them.
/// <br>
/// Checks include:
/// - File name / class name match. If they don't, the class will be marked, JavaClass::classNameMismatch.
/// - Class access flags will be checked for invalid flags combination, possibly setting the class status to
/// INVALID_ACCESS_FLAGS or USE_OF_RESERVED_CLASS_ACCESS_FLAGS.
/// - All fields and methods access flags will be checked for invalid flags combination, possibly setting the class
/// status to USE_OF_RESERVED_FIELD_ACCESS_FLAGS, USE_OF_RESERVED_METHOD_ACCESS_FLAGS or INVALID_ACCESS_FLAGS.
/// - Class "this_class" and "super_class" indexes will be checked to see if they point to a valid CONSTANT_Class
/// entry in the constant pool, possibly setting the class status to INVALID_THIS_CLASS_INDEX or INVALID_SUPER_CLASS_INDEX.
/// - Constant pool entries with CONSTANT_Class tag will be checked if name_index points to a valid CONSTANT_Utf8
/// entry that contains a valid class name, possibly setting the class status to INVALID_NAME_INDEX.
/// - Constant pool entries with CONSTANT_String tag will be checked if string_index points to a valid CONSTANT_Utf8
/// entry, possibly setting the class status to INVALID_STRING_INDEX.
/// - Contant pool entries with CONSTANT_Methodref or CONSTANT_InterfaceMethodref tag will be checked if name_index points
/// a valid CONSTANT_Utf8 entry, possibly setting the class status to INVALID_CLASS_INDEX. name_and_type index will also
/// be checked to see if they point to a valid CONSTANT_NameAndType. The name_index and descriptor_index of the
/// CONSTANT_NameAndType will be checked to see if it they point to a CONSTANT_Utf8 index. The name_index will be checked
/// to see if it holds a valid method name, and the descriptor will be checked to see if it holds a valid method descriptor.
/// Possible status setting due to errors are: INVALID_CLASS_INDEX, INVALID_NAME_INDEX and INVALID_METHOD_DESCRIPTOR_INDEX.
/// - Contant pool entries with CONSTANT_Fieldref tag will be checked if name_index points a valid CONSTANT_Utf8 entry,
/// possibly setting the class status to INVALID_CLASS_INDEX. name_and_type index will also
/// be checked to see if they point to a valid CONSTANT_NameAndType. The name_index and descriptor_index of the
/// CONSTANT_NameAndType will be checked to see if it they point to a CONSTANT_Utf8 index. The name_index will be checked
/// to see if it holds a valid field name, and the descriptor will be checked to see if it holds a valid field descriptor.
/// Possible status setting due to errors are: INVALID_CLASS_INDEX, INVALID_NAME_INDEX and INVALID_FIELD_DESCRIPTOR_INDEX.
/// - Every time something is read from the class file, it is checked if an End Of File was found too soon. The class status
/// will be changed if an EOF was unexpected. Possible status setting due to EOF related errors are: UNEXPECTED_EOF,
/// UNEXPECTED_EOF_READING_CONSTANT_POOL, UNEXPECTED_EOF_READING_UTF8, UNEXPECTED_EOF_READING_INTERFACES and
/// UNEXPECTED_EOF_READING_ATTRIBUTE_INFO.
/// - Memory allocation failures will set the class status to MEMORY_ALLOCATION_FAILED.
/// - 0xCAFEBABE signature will be checked, along with file version. Possible status setting because of this are:
/// CLASS_STATUS_UNSUPPORTED_VERSION or CLASS_STATUS_INVALID_SIGNATURE.
/// - By the end of the class file reading, if there are more data than expected, class status will be changed to
/// FILE_CONTAINS_UNEXPECTED_DATA.
/// - All interfaces' index read are checked to see if they point to a valid CONSTANT_Class entry in the constant pool,
/// possibly setting the class status to INVALID_INTERFACE_INDEX.
/// - All UTF-8 streams are checked for invalid bytes during their extraction from the file, possibly setting the class
/// status to INVALID_UTF8_BYTES.
///
/// All those status code are defined in javaclass.h, see @ref JavaClassStatus for all status.
/// Errors description can be obtained with the function decodeJavaClassStatus().
///
/// Some other checks while reading attributes (from classes, fields and methods) are also made,
/// possibly setting the class status to:
/// ATTRIBUTE_LENGTH_MISMATCH, ATTRIBUTE_INVALID_CONSTANTVALUE_INDEX, ATTRIBUTE_INVALID_SOURCEFILE_INDEX,
/// ATTRIBUTE_INVALID_INNERCLASS_INDEXES, ATTRIBUTE_INVALID_EXCEPTIONS_CLASS_INDEX or ATTRIBUTE_INVALID_CODE_LENGTH.
///
/// There is currently no verification made on instruction parameters and validity of bytecode.
///
///
///
/// @section limitations Limitations
/// This software must be compiled in 32-bit mode, as pointer are cast to/from integers of 32 bits.
///
/// There is no garbage collector, so all memory allocated for objects created will be released only when
/// the program terminates.
///
/// There are a few instructions that haven't been implemented. They are listed below:
///     - invokedynamic - will produce error if executed
///     - checkcast - will produce error if executed
///     - instanceof - will produce error if executed
///     - athrow - will produce error if executed
///     - monitorenter - ignored
///     - monitorexit - ignored
///
/// There is also no support for exceptions nor try/catch/finally blocks.
/// Class "java/lang/System" and "java/lang/String" can be used, but they are
/// also limited.
/// <br>
/// It is possible to print data to stdout using java/lang/System.out.println(),
/// but all other System's methods are unavailable.
/// <br>
/// Strings have no methods implemented, therefore they can only be created
/// and printed. Other common instructions that deal with with objects will
/// also work for strings, like comparing them with 'null'.
/// <br>
/// Parameter passing from command line to the running java program is not possible.
