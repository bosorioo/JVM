#include "attributes.h"
#include "readfunctions.h"
#include "utf8.h"
#include <stdlib.h>
#include <inttypes.h> // Usage of macro "PRId64" to print 64 bit integer

char readAttribute(JavaClassFile* jcf, attribute_info* entry)
{
    entry->info = NULL;

    if (!readu2(jcf, &entry->name_index) ||
        !readu4(jcf, &entry->length))
    {
        jcf->status = UNEXPECTED_EOF;
        return 0;
    }

    // Checks if name_index points to a valid name index in
    // the constant pool

    if (entry->name_index == 0 ||
        entry->name_index >= jcf->constantPoolCount ||
        jcf->constantPool[entry->name_index - 1].tag != CONSTANT_Utf8)
    {
        jcf->status = INVALID_NAME_INDEX;
        return 0;
    }

    if (entry->length > 0)
    {
        entry->info = (uint8_t*)malloc(entry->length);

        if (!entry->info)
        {
            jcf->status = MEMORY_ALLOCATION_FAILED;
            return 0;
        }

        uint32_t i;
        int byte;

        for (i = 0; i < entry->length; i++)
        {
            byte = fgetc(jcf->file);

            if (byte == EOF)
            {
                jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
                return 0;
            }

            *(entry->info + i) = (uint8_t)byte;
            jcf->totalBytesRead++;
        }
    }


    return 1;
}

void freeAttributeInfo(attribute_info* entry)
{
    if (entry->info)
    {
        free(entry->info);
        entry->info = NULL;
    }
}

void ident(int level)
{
    while (level-- > 0)
        printf("\t");
}

void printAttributeConstantValue(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    uint16_t constantvalue_index;
    cp_info* cp;
    char buffer[48];

    constantvalue_index = (uint16_t)(*entry->info) << 8;
    constantvalue_index += *(entry->info + 1);
    cp = jcf->constantPool + constantvalue_index - 1;

    ident(identationLevel);
    printf("constantvalue_index: #%u <", constantvalue_index);

    switch (cp->tag)
    {
        case CONSTANT_Integer:
            printf("%d", (int32_t)cp->Integer.value);
            break;

        case CONSTANT_Long:
            printf("%" PRId64, ((int64_t)cp->Long.high << 32) | cp->Long.low);
            break;

        case CONSTANT_Float:
            printf("%e", readConstantPoolFloat(cp));
            break;

        case CONSTANT_Double:
            printf("%e", readConstantPoolDouble(cp));
            break;

        case CONSTANT_String:
            cp = jcf->constantPool + cp->String.string_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            break;

        default:
            printf(" - unknown constant tag - ");
            break;
    }

    printf(">");
}

void printAttributeSourceFile(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    uint16_t sourcefile_index;
    cp_info* cp;
    char buffer[48];

    sourcefile_index = (uint16_t)(*entry->info) << 8;
    sourcefile_index += *(entry->info + 1);
    cp = jcf->constantPool + sourcefile_index - 1;
    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);

    ident(identationLevel);
    printf("sourcefile_index: #%u <%s>", sourcefile_index, buffer);
}

void printAttributeInnerClasses(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    uint16_t number_of_classes, u16, index;
    uint8_t* ptr = entry->info;
    cp_info* cp;
    char buffer[48];

    number_of_classes = (uint16_t)(*ptr++) << 8;
    number_of_classes += *ptr++;

    ident(identationLevel);
    printf("number_of_classes: %u", number_of_classes);

    for (index = 0; index < number_of_classes; index++)
    {
        printf("\n\n");
        ident(identationLevel);
        printf("Inner Class #%u:\n", index + 1);

        // inner_class_index
        u16 = (uint16_t)((*ptr++) << 8); u16 += (*ptr++);
        cp = jcf->constantPool + u16 - 1;
        cp = jcf->constantPool + cp->Class.name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
        ident(identationLevel + 1);
        printf("inner_class_info_index:   cp index #%u <%s>\n", u16, buffer);

        // outer_class_index
        u16 = (uint16_t)((*ptr++) << 8); u16 += (*ptr++);
        ident(identationLevel + 1);
        printf("outer_class_info_index:   cp index #%u ", u16);

        if (u16 == 0)
        {
            printf("(no outer class)\n");
        }
        else
        {
            cp = jcf->constantPool + u16 - 1;
            cp = jcf->constantPool + cp->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("<%s>\n", buffer);
        }

        // inner_class_name_index
        u16 = (uint16_t)((*ptr++) << 8); u16 += (*ptr++);
        ident(identationLevel + 1);
        printf("inner_name_index:         cp index #%u ", u16);

        if (u16 == 0)
        {
            printf("(anonymous class)\n");
        }
        else
        {
            cp = jcf->constantPool + u16 - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("<%s>\n", buffer);
        }

        // inner_class_access_flags
        u16 = (uint16_t)((*ptr++) << 8); u16 += (*ptr++);
        decodeAccessFlags(u16, buffer, sizeof(buffer), ACCT_CLASS);
        ident(identationLevel + 1);
        printf("inner_class_access_flags: 0x%.4X [%s]", u16, buffer);
    }
}

void printAttribute(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    cp_info* cp = jcf->constantPool + entry->name_index - 1;

    #define CMP_ATT(name) cmp_UTF8_Ascii(cp->Utf8.bytes, cp->Utf8.length, (uint8_t*)name, sizeof(name) - 1)

    if (CMP_ATT("ConstantValue"))
    {
        printAttributeConstantValue(jcf, entry, identationLevel);
    }
    else if (CMP_ATT("SourceFile"))
    {
        printAttributeSourceFile(jcf, entry, identationLevel);
    }
    else if (CMP_ATT("InnerClasses"))
    {
        printAttributeInnerClasses(jcf, entry, identationLevel);
    }
    else
    {
        ident(identationLevel);
        printf("Attribute not implemented and ignored.");
    }

    #undef CMP_ATT
}

void printAllAttributes(JavaClassFile* jcf)
{
    if (jcf->attributeCount == 0)
        return;

    uint16_t u16;
    char buffer[48];
    cp_info* cp;
    attribute_info* atti;

    printf("\n---- Attributes ----");

    for (u16 = 0; u16 < jcf->attributeCount; u16++)
    {
        atti = jcf->attributes + u16;
        cp = jcf->constantPool + atti->name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);

        printf("\n\n\tAttribute #%u - %s\n", u16 + 1, buffer);
        printAttribute(jcf, atti, 2);
    }
}
