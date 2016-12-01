#include "validity.h"
#include "constantpool.h"
#include "utf8.h"
#include "readfunctions.h"
#include <locale.h>
#include <wctype.h>
#include <ctype.h>

/// @brief Checks whether the "accessFlags" parameter has a valid combination
/// of method flags.
///
/// @param JavaClass* jc - pointer to the class
/// @param uint16_t accessFlags - flag to be analyzed
///
/// @return char - in case of issues, jc->status is changed, and the
/// function returns 0. Otherwise, nothing is changed with jc, and the
/// return value is 1.
char checkMethodAccessFlags(JavaClass* jc, uint16_t accessFlags)
{
    if (accessFlags & ACC_INVALID_METHOD_FLAG_MASK)
    {
        jc->status = USE_OF_RESERVED_METHOD_ACCESS_FLAGS;
        return 0;
    }

    // If the method is ABSTRACT, it must not be FINAL, NATIVE, PRIVATE, STATIC, STRICT nor SYNCHRONIZED
    if ((accessFlags & ACC_ABSTRACT) &&
        (accessFlags & (ACC_FINAL | ACC_NATIVE | ACC_PRIVATE | ACC_STATIC | ACC_STRICT | ACC_SYNCHRONIZED)))
    {
        jc->status = INVALID_ACCESS_FLAGS;
        return 0;
    }

    uint8_t accessModifierCount = 0;

    if (accessFlags & ACC_PUBLIC)
        accessModifierCount++;

    if (accessFlags & ACC_PRIVATE)
        accessModifierCount++;

    if (accessFlags & ACC_PROTECTED)
        accessModifierCount++;

    // A method can't be more than one of those: PUBLIC, PRIVATE, PROTECTED
    if (accessModifierCount > 1)
    {
        jc->status = INVALID_ACCESS_FLAGS;
        return 0;
    }

    return 1;
}

/// @brief Checks whether the "accessFlags" parameter has a valid combination
/// of field flags.
///
/// @param JavaClass* jc - pointer to the class
/// @param uint16_t accessFlags - flag to be analyzed
///
/// @return char - in case of issues, jc->status is changed, and the
/// function returns 0. Otherwise, nothing is changed with jc, and the
/// return value is 1.
char checkFieldAccessFlags(JavaClass* jc, uint16_t accessFlags)
{
    if (accessFlags & ACC_INVALID_FIELD_FLAG_MASK)
    {
        jc->status = USE_OF_RESERVED_FIELD_ACCESS_FLAGS;
        return 0;
    }

    // The field can't be both ABSTRACT and FINAL
    if ((accessFlags & ACC_ABSTRACT) && (accessFlags & ACC_FINAL))
    {
        jc->status = INVALID_ACCESS_FLAGS;
        return 0;
    }

    uint16_t interfaceRequiredBitMask = ACC_PUBLIC | ACC_STATIC | ACC_FINAL;

    // If the class is an interface, then the bits PUBLIC, STATIC and FINAL must
    // be set to 1 for all fields.
    if ((jc->accessFlags & ACC_INTERFACE) &&
        ((jc->accessFlags & interfaceRequiredBitMask) != interfaceRequiredBitMask))
    {
        jc->status = INVALID_ACCESS_FLAGS;
        return 0;
    }

    uint8_t accessModifierCount = 0;

    if (accessFlags & ACC_PUBLIC)
        accessModifierCount++;

    if (accessFlags & ACC_PRIVATE)
        accessModifierCount++;

    if (accessFlags & ACC_PROTECTED)
        accessModifierCount++;

    // A field can't be more than one of those: PUBLIC, PRIVATE, PROTECTED
    if (accessModifierCount > 1)
    {
        jc->status = INVALID_ACCESS_FLAGS;
        return 0;
    }

    return 1;
}

/// @brief Checks whether the "accessFlags" from the class file is a valid
/// combination of class flags.
///
/// This function also checks if "thisClass" and
/// "superClass" points to valid class indexes.
///
/// @param JavaClass* jc - pointer to class to be analyzed
///
/// @return char - in case of issues, jc->status is changed, and the
/// function returns 0. Otherwise, nothing is changed with jc, and the
/// return value is 1. This function also
char checkClassIndexAndAccessFlags(JavaClass* jc)
{
    if (jc->accessFlags & ACC_INVALID_CLASS_FLAG_MASK)
    {
        jc->status = USE_OF_RESERVED_CLASS_ACCESS_FLAGS;
        return 0;
    }

    // If the class is an interface, then it must be abstract.
    // Interface must have the ACC_FINAL and ACC_SUPER bits set to 0.
    if (jc->accessFlags & ACC_INTERFACE)
    {
        if ((jc->accessFlags & ACC_ABSTRACT) == 0 ||
            (jc->accessFlags & (ACC_FINAL | ACC_SUPER)))
        {
            jc->status = INVALID_ACCESS_FLAGS;
            return 0;
        }
    }

    // "thisClass" must not be 0, can't point out of CP bounds and must
    // point to a class index.
    if (!jc->thisClass || jc->thisClass >= jc->constantPoolCount ||
        jc->constantPool[jc->thisClass - 1].tag != CONSTANT_Class)
    {
        jc->status = INVALID_THIS_CLASS_INDEX;
        return 0;
    }

    // "superClass" can't be out of CP bounds.
    // It can be zero, but if it''s not, then it must point to a
    // class index.
    if (jc->superClass >= jc->constantPoolCount ||
        (jc->superClass && jc->constantPool[jc->superClass - 1].tag != CONSTANT_Class))
    {
        jc->status = INVALID_SUPER_CLASS_INDEX;
        return 0;
    }

    return 1;
}

/// @brief Checks whether the name of the class pointed by "thisClass" is the same name
/// as the class file.
///
/// This function doesn't check package and folders. It
/// basically gets the name of file, for instance: "C:/mypath/File.class" will be
/// understood as "File". The name of the class could be something like: "package/MyClass",
/// the package will be ignored and function will understand the class name as "MyClass".
/// The function then proceeds to compare "MyClass" and "File".
///
/// @param JavaClass* jc - pointer to class to be analyzed
///
/// @return char - If there is a mismatch, jc->status is modified and the function
/// returns 0. Otherwise, it is left unchanged and the return value is 1.
char checkClassNameFileNameMatch(JavaClass* jc, const char* classFilePath)
{
    int32_t i, begin = 0, end;
    cp_info* cpi;

    // This is to get the indexes of the last '/' (to ignore folders)
    // and the first '.' (to separe the .class suffix, in case it exists).
    for (i = 0; classFilePath[i] != '\0'; i++)
    {
        if (classFilePath[i] == '/' || classFilePath[i] == '\\')
            begin = i + 1;
        else if (classFilePath[i] == '.')
            break;
    }

    end = i;

    cpi = jc->constantPool + jc->thisClass - 1;
    cpi = jc->constantPool + cpi->Class.name_index - 1;

    for (i = 0; i < cpi->Utf8.length; i++)
    {
        if (*(cpi->Utf8.bytes + i) == '/')
        {
            if (begin == 0)
                break;

            while (--begin > 0 && (classFilePath[begin - 1] != '/' || classFilePath[begin - 1] != '\\'));
        }
    }

    return cmp_UTF8_FilePath(cpi->Utf8.bytes, cpi->Utf8.length, (uint8_t*)classFilePath + begin, end - begin);
}

/// @brief Checks whether the UTF-8 stream is a valid Java Identifier.
///
/// A java identifer (class name, variable name, etc) must start with underscore, dollar sign
/// or letter, and could finish with numbers, letters, dollar sign or underscore.
///
/// @param uint8_t* utf8_bytes - pointer to the sequence of bytes that identify the class name
/// @param int32_t utf8_len - legth of class name
/// @param uint8_t isClassIdentifier - is set to anything but zero, then this function will also
/// accept slashes (/), as classes have their full path separated by slashes.
///
/// @ return char - The function returns 1 if it the string is indeed
/// a valid Java Identifier, otherwise zero.
char isValidJavaIdentifier(uint8_t* utf8_bytes, int32_t utf8_len, uint8_t isClassIdentifier)
{
    uint32_t utf8_char;
    uint8_t used_bytes;
    uint8_t firstChar = 1;
    char isValid = 1;

    if (*utf8_bytes == '[')
        return readFieldDescriptor(utf8_bytes, utf8_len, 1) == utf8_len;

    while (utf8_len > 0)
    {
        used_bytes = nextUTF8Character(utf8_bytes, utf8_len, &utf8_char);

        if (used_bytes == 0)
        {
            isValid = 0;
            break;
        }

        if (isalpha(utf8_char) || utf8_char == '_' || utf8_char == '$' ||
            (isdigit(utf8_char) && !firstChar) || iswalpha(utf8_char) ||
            (utf8_char == '/' && !firstChar && isClassIdentifier))
        {
            firstChar = utf8_char == '/';
            utf8_len -= used_bytes;
            utf8_bytes += used_bytes;
        }
        else
        {
            isValid = 0;
            break;
        }
    }

    return isValid;
}

/// @brief Identifies if a index in JavaClass is valid
///
/// @param JavaClass* jc - pointer to the class
/// @param uint16_t index - represents the actual index
///
/// @return char - 1 if the index points to a UTF-8 constant, 0 otherwise.
char isValidUTF8Index(JavaClass* jc, uint16_t index)
{
    return (jc->constantPool + index - 1)->tag == CONSTANT_Utf8;
}

// Returns 1 if "name_index" points to a valid UTF-8 and is a valid
// java identifier. The parameter "isClassIdentifier", if set to zero,
// will not accept slashes (/) as valid identifier characters. If
// "isClassIdentifier" is different than zero, then classes with slashes
// in its name like "java/io/Stream" will be accepted as valid names.
// Returns 0 otherwise.
char isValidNameIndex(JavaClass* jc, uint16_t name_index, uint8_t isClassIdentifier)
{
    cp_info* entry = jc->constantPool + name_index - 1;
    return entry->tag == CONSTANT_Utf8 && isValidJavaIdentifier(entry->Utf8.bytes, entry->Utf8.length, isClassIdentifier);
}

// Returns 1 if the name index points to a valid UTF-8 and is a valid
// java  identifier. The difference between this function and "isValidNameIndex"
// is that this function accepts "<init>" and "<clinit>" as valid names for methods,
// whereas "isValidNameIndex" (consenquently "isValidJavaIdentifier") function wouldn't
// accept those names as valid. Returns zero otherwise.
char isValidMethodNameIndex(JavaClass* jc, uint16_t name_index)
{
    cp_info* entry = jc->constantPool + name_index - 1;

    if (entry->tag != CONSTANT_Utf8)
        return 0;

    if (cmp_UTF8_Ascii(entry->Utf8.bytes, entry->Utf8.length, (uint8_t*)"<init>", 6) ||
        cmp_UTF8_Ascii(entry->Utf8.bytes, entry->Utf8.length, (uint8_t*)"<clinit>", 8))
        return 1;

    return isValidJavaIdentifier(entry->Utf8.bytes, entry->Utf8.length, 0);
}

// Checks whether the index points to a valid class and whether that
// class name is valid. In case the check fails, jc->status is changed and
// the function returns 0. If everything is successful, the function returns 1
// while leaving jc unchanged.
char checkClassIndex(JavaClass* jc, uint16_t class_index)
{
    cp_info* entry = jc->constantPool + class_index - 1;

    if (entry->tag != CONSTANT_Class || !isValidNameIndex(jc, entry->Class.name_index, 1))
    {
        jc->status = INVALID_CLASS_INDEX;
        return 0;
    }

    return 1;
}

// Checks whether name_and_type_index points to a valid CONSTANT_NameAndType,
// checking if the name and the descriptor are valid. The descriptor must be
// a valid field descriptor.
// In case of failure, the function changes jc->status and returns 0.
// Otherwise, jc is left unchanged, and the function returns 1.
char checkFieldNameAndTypeIndex(JavaClass* jc, uint16_t name_and_type_index)
{
    cp_info* entry = jc->constantPool + name_and_type_index - 1;

    if (entry->tag != CONSTANT_NameAndType || !isValidNameIndex(jc, entry->NameAndType.name_index, 0))
    {
        jc->status = INVALID_NAME_INDEX;
        return 0;
    }

    entry = jc->constantPool + entry->NameAndType.descriptor_index - 1;

    if (entry->tag != CONSTANT_Utf8 || entry->Utf8.length == 0)
    {
        jc->status = INVALID_FIELD_DESCRIPTOR_INDEX;
        return 0;
    }

    if (entry->Utf8.length != readFieldDescriptor(entry->Utf8.bytes, entry->Utf8.length, 1))
    {
        jc->status = INVALID_FIELD_DESCRIPTOR_INDEX;
        return 0;
    }

    return 1;
}

// Checks whether name_and_type_index points to a valid CONSTANT_NameAndType,
// checking if the name and the descriptor are valid. The descriptor must be
// a valid method descriptor.
// In case of failure, the function changes jc->status and returns 0.
// Otherwise, jc is left unchanged, and the function returns 1.
char checkMethodNameAndTypeIndex(JavaClass* jc, uint16_t name_and_type_index)
{
    cp_info* entry = jc->constantPool + name_and_type_index - 1;

    if (entry->tag != CONSTANT_NameAndType || !isValidMethodNameIndex(jc, entry->NameAndType.name_index))
    {
        jc->status = INVALID_NAME_INDEX;
        return 0;
    }

    entry = jc->constantPool + entry->NameAndType.descriptor_index - 1;

    if (entry->tag != CONSTANT_Utf8 || entry->Utf8.length == 0)
    {
        jc->status = INVALID_METHOD_DESCRIPTOR_INDEX;
        return 0;
    }

    if (entry->Utf8.length != readMethodDescriptor(entry->Utf8.bytes, entry->Utf8.length, 1))
    {
        jc->status = INVALID_METHOD_DESCRIPTOR_INDEX;
        return 0;
    }

    return 1;
}


// Iterates over the constant pool looking for inconsistencies in it.
// If no error is encountered, the function returns 1. In case of failures,
// the function will set jc->status accordingly and will return 0.
char checkConstantPoolValidity(JavaClass* jc)
{
    uint16_t i;
    char success = 1;
    char* previousLocale = setlocale(LC_CTYPE, "pt_BR.UTF-8");

    for (i = 0; success && i < jc->constantPoolCount - 1; i++)
    {
        cp_info* entry = jc->constantPool + i;

        switch(entry->tag)
        {
            case CONSTANT_Class:

                if (!isValidNameIndex(jc, entry->Class.name_index, 1))
                {
                    jc->status = INVALID_NAME_INDEX;
                    success = 0;
                }

                break;

            case CONSTANT_String:

                if (!isValidUTF8Index(jc, entry->String.string_index))
                {
                    jc->status = INVALID_STRING_INDEX;
                    success = 0;
                }

                break;

            case CONSTANT_Methodref:
            case CONSTANT_InterfaceMethodref:

                if (!checkClassIndex(jc, entry->Methodref.class_index) ||
                    !checkMethodNameAndTypeIndex(jc, entry->Methodref.name_and_type_index))
                {
                    success = 0;
                }

                break;

            case CONSTANT_Fieldref:

                if (!checkClassIndex(jc, entry->Fieldref.class_index) ||
                    !checkFieldNameAndTypeIndex(jc, entry->Fieldref.name_and_type_index))
                {
                    success = 0;
                }

                break;

            case CONSTANT_NameAndType:

                if (!isValidUTF8Index(jc, entry->NameAndType.name_index) ||
                    !isValidUTF8Index(jc, entry->NameAndType.descriptor_index))
                {
                    jc->status = INVALID_NAME_AND_TYPE_INDEX;
                    success = 0;
                }

                break;

            case CONSTANT_Double:
            case CONSTANT_Long:
                // Doubles and long take two entries in the constant pool
                i++;
                break;

            case CONSTANT_Float:
            case CONSTANT_Integer:
            case CONSTANT_Utf8:
                // There's nothing to check for these constant types, since they
                // don't have pointers to other constant pool entries.
                break;

            default:
                // Will never happen, since this kind of error would be detected
                // earlier, while reading the constant pool from .class file.
                break;
        }

        jc->validityEntriesChecked = i + 1;
    }

    setlocale(LC_CTYPE, previousLocale);

    return success;
}
