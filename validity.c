#include "validity.h"
#include "constantpool.h"
#include "utf8.h"
#include "ctype.h"
#include "locale.h"

char checkClassName(JavaClassFile* jcf, const char* classFilePath)
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
            return 0;

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

    return result;
}

char isValidJavaIdentifier(uint8_t* utf8_bytes, int32_t utf8_len, uint8_t acceptSlashes)
{
    uint32_t utf8_char;
    uint8_t used_bytes;
    uint8_t firstChar = 1;
    char isValid = 1;
    char* previousLocale = setlocale(LC_CTYPE, "");

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
            (utf8_char == '/' && acceptSlashes))
        {
            if (utf8_char == '/')
                firstChar = 1;
            else
                firstChar = 0;

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

char isValidNameIndex(JavaClassFile* jcf, uint16_t name_index, uint8_t acceptSlashes)
{
    cp_info* entry = jcf->constantPool + name_index - 1;
    return entry->tag == CONSTANT_Utf8 && isValidJavaIdentifier(entry->Utf8.bytes, entry->Utf8.length, acceptSlashes);
}

char checkClassIndex(JavaClassFile* jcf, uint16_t class_index)
{
    cp_info* entry = jcf->constantPool + class_index - 1;
    return entry->tag == CONSTANT_Class && isValidNameIndex(jcf, entry->Class.name_index, 1);
}

char checkFieldNameAndTypeIndex(JavaClassFile* jcf, uint16_t name_and_type_index)
{
    cp_info* entry = jcf->constantPool + name_and_type_index - 1;

    if (entry->tag != CONSTANT_NameAndType || !isValidNameIndex(jcf, entry->NameAndType.name_index, 0))
        return 0;


    // TODO: check that the descriptor is a valid Field Descriptor

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

            case CONSTANT_Fieldref:

                if (!checkClassIndex(jcf, entry->Fieldref.class_index))
                {
                    jcf->status = INVALID_CLASS_INDEX;
                    return 0;
                }

                if (!checkFieldNameAndTypeIndex(jcf, entry->Fieldref.name_and_type_index))
                {
                    jcf->status = INVALID_FIELD_DESCRIPTOR_INDEX;
                    return 0;
                }

                break;

            // TODO: complete with other cases

            default:
                // Will never happen, since this kind of error would be detected
                // earlier, while reading the constant pool from .class file.
                break;
        }
    }

    return 1;
}
