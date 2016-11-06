#include "validity.h"
#include "constantpool.h"
#include "utf8.h"
#include "ctype.h"
#include "readfunctions.h"
#include "locale.h"

char checkMethodAccessFlags(JavaClassFile* jcf, uint16_t accessFlags)
{
    if (accessFlags & ACC_INVALID_METHOD_FLAG_MASK)
    {
        jcf->status = USE_OF_RESERVED_METHOD_ACCESS_FLAGS;
        return 0;
    }

    // TODO: check access flags validity, example: can't be PRIVATE and PUBLIC

    return 1;
}

char checkClassIndexAndAccessFlags(JavaClassFile* jcf)
{
    if (jcf->accessFlags & ACC_INVALID_CLASS_FLAG_MASK)
    {
        jcf->status = USE_OF_RESERVED_CLASS_ACCESS_FLAGS;
        return 0;
    }

    // If the class is abstract, then it must be an interface.
    // If the class is an interface, then it must be abstract.
    // Abstract/interface must have the ACC_FINAL and ACC_SUPER bits set to 0.
    if ((jcf->accessFlags & ACC_ABSTRACT) || (jcf->accessFlags & ACC_INTERFACE))
    {
        if ((jcf->accessFlags & ACC_ABSTRACT) == 0 ||
            (jcf->accessFlags & ACC_INTERFACE) == 0 ||
            (jcf->accessFlags & (ACC_FINAL | ACC_SUPER)))
        {
            jcf->status = INVALID_ACCESS_FLAGS;
            return 0;
        }
    }

    if (!jcf->thisClass || jcf->constantPool[jcf->thisClass - 1].tag != CONSTANT_Class)
    {
        jcf->status = INVALID_THIS_CLASS_INDEX;
        return 0;
    }

    if (jcf->superClass && jcf->constantPool[jcf->superClass - 1].tag != CONSTANT_Class)
    {
        jcf->status = INVALID_SUPER_CLASS_INDEX;
        return 0;
    }

    return 1;
}

char checkClassNameFileNameMatch(JavaClassFile* jcf, const char* classFilePath)
{
    int32_t i, begin = 0, end;
    char result;

    for (i = 0; classFilePath[i] != '\0'; i++)
    {
        if (classFilePath[i] == '/')
            begin = i + 1;
        else if (classFilePath[i] == '.')
            break;
    }

    end = i;
    i = 0;
    result = 1;

    cp_info* entry = jcf->constantPool + jcf->thisClass - 1;
    entry = jcf->constantPool + entry->Class.name_index - 1;

    uint8_t* utf8_class_name = entry->Utf8.bytes;
    int32_t utf8_len = entry->Utf8.length;

    uint32_t utf8_char;
    uint8_t bytes_used;

    while (utf8_len > 0)
    {
        bytes_used = nextUTF8Character(utf8_class_name, utf8_len, &utf8_char);

        if (bytes_used == 0)
        {
            jcf->status = CLASS_NAME_FILE_NAME_MISMATCH;
            return 0;
        }

        utf8_class_name += bytes_used;
        utf8_len -= bytes_used;

        if (utf8_char == '/')
        {
            i = 0;
            result = 1;
        }
        else
        {
            if (result)
                result = (utf8_char <= 255) && (i < end) && ((char)utf8_char == classFilePath[begin + i]);

            i++;
        }
    }

    if (result)
        result = i == (end - begin);

    if (result == 0)
        jcf->status = CLASS_NAME_FILE_NAME_MISMATCH;

    return result;
}

char isValidJavaIdentifier(uint8_t* utf8_bytes, int32_t utf8_len, uint8_t isClassIdentifier)
{
    uint32_t utf8_char;
    uint8_t used_bytes;
    uint8_t firstChar = 1;
    char isValid = 1;
    char* previousLocale = setlocale(LC_CTYPE, "");

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
            (isdigit(utf8_char) && !firstChar) ||
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

    setlocale(LC_CTYPE, previousLocale);

    return isValid;
}

char isValidUTF8Index(JavaClassFile* jcf, uint16_t index)
{
    return (jcf->constantPool + index - 1)->tag == CONSTANT_Utf8;
}

char isValidNameIndex(JavaClassFile* jcf, uint16_t name_index, uint8_t isClassIdentifier)
{
    cp_info* entry = jcf->constantPool + name_index - 1;
    return entry->tag == CONSTANT_Utf8 && isValidJavaIdentifier(entry->Utf8.bytes, entry->Utf8.length, isClassIdentifier);
}

char isValidMethodNameIndex(JavaClassFile* jcf, uint16_t name_index)
{
    cp_info* entry = jcf->constantPool + name_index - 1;

    if (entry->tag != CONSTANT_Utf8)
        return 0;

    if (cmp_UTF8_Ascii(entry->Utf8.bytes, entry->Utf8.length, (uint8_t*)"<init>", 6) ||
        cmp_UTF8_Ascii(entry->Utf8.bytes, entry->Utf8.length, (uint8_t*)"<clinit>", 8))
        return 1;

    return isValidJavaIdentifier(entry->Utf8.bytes, entry->Utf8.length, 0);
}

char checkClassIndex(JavaClassFile* jcf, uint16_t class_index)
{
    cp_info* entry = jcf->constantPool + class_index - 1;

    if (entry->tag != CONSTANT_Class || !isValidNameIndex(jcf, entry->Class.name_index, 1))
    {
        jcf->status = INVALID_CLASS_INDEX;
        return 0;
    }

    return 1;
}

char checkFieldNameAndTypeIndex(JavaClassFile* jcf, uint16_t name_and_type_index)
{
    cp_info* entry = jcf->constantPool + name_and_type_index - 1;

    if (entry->tag != CONSTANT_NameAndType || !isValidNameIndex(jcf, entry->NameAndType.name_index, 0))
    {
        jcf->status = INVALID_NAME_INDEX;
        return 0;
    }

    entry = jcf->constantPool + entry->NameAndType.descriptor_index - 1;

    if (entry->tag != CONSTANT_Utf8 || entry->Utf8.length == 0)
    {
        jcf->status = INVALID_FIELD_DESCRIPTOR_INDEX;
        return 0;
    }

    if (entry->Utf8.length != readFieldDescriptor(entry->Utf8.bytes, entry->Utf8.length, 1))
    {
        jcf->status = INVALID_FIELD_DESCRIPTOR_INDEX;
        return 0;
    }

    return 1;
}

char checkMethodNameAndTypeIndex(JavaClassFile* jcf, uint16_t name_and_type_index)
{
    cp_info* entry = jcf->constantPool + name_and_type_index - 1;

    if (entry->tag != CONSTANT_NameAndType || !isValidMethodNameIndex(jcf, entry->NameAndType.name_index))
    {
        jcf->status = INVALID_NAME_INDEX;
        return 0;
    }

    entry = jcf->constantPool + entry->NameAndType.descriptor_index - 1;

    if (entry->tag != CONSTANT_Utf8 || entry->Utf8.length == 0)
    {
        jcf->status = INVALID_METHOD_DESCRIPTOR_INDEX;
        return 0;
    }

    if (entry->Utf8.length != readMethodDescriptor(entry->Utf8.bytes, entry->Utf8.length, 1))
    {
        jcf->status = INVALID_METHOD_DESCRIPTOR_INDEX;
        return 0;
    }

    return 1;
}

char checkConstantPoolValidity(JavaClassFile* jcf)
{
    uint16_t i;

    for (i = 0; i < jcf->constantPoolCount - 1; i++)
    {
        jcf->currentConstantPoolEntryIndex = i;
        cp_info* entry = jcf->constantPool + i;

        switch(entry->tag)
        {
            case CONSTANT_Class:

                if (!isValidNameIndex(jcf, entry->Class.name_index, 1))
                {
                    jcf->status = INVALID_NAME_INDEX;
                    return 0;
                }

                break;

            case CONSTANT_String:

                if (!isValidUTF8Index(jcf, entry->String.string_index))
                {
                    jcf->status = INVALID_STRING_INDEX;
                    return 0;
                }

                break;

            case CONSTANT_Methodref:
            case CONSTANT_InterfaceMethodref:

                if (!checkClassIndex(jcf, entry->Methodref.class_index))
                    return 0;

                if (!checkMethodNameAndTypeIndex(jcf, entry->Methodref.name_and_type_index))
                    return 0;

                break;

            case CONSTANT_Fieldref:

                if (!checkClassIndex(jcf, entry->Fieldref.class_index))
                    return 0;

                if (!checkFieldNameAndTypeIndex(jcf, entry->Fieldref.name_and_type_index))
                    return 0;

                break;

            case CONSTANT_NameAndType:

                if (!isValidUTF8Index(jcf, entry->NameAndType.name_index) ||
                    !isValidUTF8Index(jcf, entry->NameAndType.descriptor_index))
                {
                    jcf->status = INVALID_NAME_AND_TYPE_INDEX;
                    return 0;
                }

                break;

            case CONSTANT_Double:
            case CONSTANT_Float:
            case CONSTANT_Integer:
            case CONSTANT_Long:
            case CONSTANT_Utf8:
                // There's nothing to check for these constant types, since they
                // don't have pointers to other constant pool entries.
                break;

            default:
                // Will never happen, since this kind of error would be detected
                // earlier, while reading the constant pool from .class file.
                break;
        }
    }

    return 1;
}
