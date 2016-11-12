#include "attributes.h"
#include "readfunctions.h"
#include "utf8.h"
#include <stdlib.h>
#include <inttypes.h> // Usage of macro "PRId64" to print 64 bit integer

#define DECLARE_ATTR_FUNCS(attr) \
    uint8_t readAttribute##attr(JavaClassFile* jcf, attribute_info* entry); \
    void printAttribute##attr(JavaClassFile* jcf, attribute_info* entry, int identationLevel); \
    void freeAttribute##attr(attribute_info* entry);

DECLARE_ATTR_FUNCS(SourceFile)
DECLARE_ATTR_FUNCS(InnerClasses)
DECLARE_ATTR_FUNCS(LineNumberTable)
DECLARE_ATTR_FUNCS(ConstantValue)
DECLARE_ATTR_FUNCS(Code)

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
    cp_info* cp = jcf->constantPool + entry->name_index - 1;

    if (entry->name_index == 0 ||
        entry->name_index >= jcf->constantPoolCount ||
        cp->tag != CONSTANT_Utf8)
    {
        jcf->status = INVALID_NAME_INDEX;
        return 0;
    }

    #define CMP_ATT(name) cmp_UTF8_Ascii(cp->Utf8.bytes, cp->Utf8.length, (uint8_t*)#name, sizeof(#name) - 1) \
        && (entry->attributeType = ATTR_##name)

    uint32_t totalBytesRead = jcf->totalBytesRead;
    char result;

    if (CMP_ATT(ConstantValue))
        result = readAttributeConstantValue(jcf, entry);
    else if (CMP_ATT(SourceFile))
        result =  readAttributeSourceFile(jcf, entry);
    else if (CMP_ATT(InnerClasses))
        result =  readAttributeInnerClasses(jcf, entry);
    else if (CMP_ATT(LineNumberTable))
        result =  readAttributeLineNumberTable(jcf, entry);
    else if (CMP_ATT(Code))
        result =  readAttributeCode(jcf, entry);
    else
    {
        uint32_t u32;

        for (u32 = 0; u32 < entry->length; u32++)
        {
            if (fgetc(jcf->file) == EOF)
            {
                jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
                return 0;
            }

            jcf->totalBytesRead++;
        }

        result = 1;
        entry->attributeType = ATTR_Unknown;
    }

    if (jcf->totalBytesRead - totalBytesRead != entry->length)
    {
        jcf->status = ATTRIBUTE_LENGTH_MISMATCH;
        return 0;
    }

    #undef CMP_ATT
    return result;
}

void ident(int level)
{
    while (level-- > 0)
        printf("\t");
}

uint8_t readAttributeConstantValue(JavaClassFile* jcf, attribute_info* entry)
{
    if (entry->length != 2)
    {
        jcf->status = ATTRIBUTE_LENGTH_MISMATCH;
        return 0;
    }

    att_ConstantValue_info* info = (att_ConstantValue_info*)malloc(sizeof(att_ConstantValue_info));
    entry->info = (void*)info;

    if (!info)
    {
        jcf->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    if (!readu2(jcf, &info->constantvalue_index))
    {
        jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    if (info->constantvalue_index == 0 ||
        info->constantvalue_index >= jcf->constantPoolCount)
    {
        jcf->status = ATTRIBUTE_INVALID_CONSTANTVALUE_INDEX;
        return 0;
    }

    cp_info* cp = jcf->constantPool + info->constantvalue_index - 1;

    if (cp->tag != CONSTANT_String && cp->tag != CONSTANT_Float &&
        cp->tag != CONSTANT_Double && cp->tag != CONSTANT_Long &&
        cp->tag != CONSTANT_Integer)
    {
        jcf->status = ATTRIBUTE_INVALID_CONSTANTVALUE_INDEX;
        return 0;
    }

    return 1;
}

void printAttributeConstantValue(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    char buffer[48];

    att_ConstantValue_info* info = (att_ConstantValue_info*)entry->info;
    cp_info* cp = jcf->constantPool + info->constantvalue_index - 1;

    ident(identationLevel);
    printf("constantvalue_index: #%u <", info->constantvalue_index);

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

void freeAttributeConstantValue(attribute_info* entry)
{
    if (entry->info)
        free(entry->info);

    entry->info = NULL;
}

uint8_t readAttributeSourceFile(JavaClassFile* jcf, attribute_info* entry)
{
    if (entry->length != 2)
    {
        jcf->status = ATTRIBUTE_LENGTH_MISMATCH;
        return 0;
    }

    att_SourceFile_info* info = (att_SourceFile_info*)malloc(sizeof(att_SourceFile_info));
    entry->info = (void*)info;

    if (!info)
    {
        jcf->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    if (!readu2(jcf, &info->sourcefile_index))
    {
        jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    if (info->sourcefile_index == 0 ||
        info->sourcefile_index >= jcf->constantPoolCount ||
        jcf->constantPool[info->sourcefile_index - 1].tag != CONSTANT_Utf8)
    {
        jcf->status = ATTRIBUTE_INVALID_SOURCEFILE_INDEX;
        return 0;
    }

    return 1;
}

void printAttributeSourceFile(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    char buffer[48];
    att_SourceFile_info* info = (att_SourceFile_info*)entry->info;
    cp_info* cp = jcf->constantPool + info->sourcefile_index - 1;

    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);

    ident(identationLevel);
    printf("sourcefile_index: #%u <%s>", info->sourcefile_index, buffer);
}

void freeAttributeSourceFile(attribute_info* entry)
{
    if (entry->info)
        free(entry->info);

    entry->info = NULL;
}

uint8_t readAttributeInnerClasses(JavaClassFile* jcf, attribute_info* entry)
{
    att_InnerClasses_info* info = (att_InnerClasses_info*)malloc(sizeof(att_InnerClasses_info));
    entry->info = (void*)info;

    if (!info)
    {
        jcf->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    info->inner_classes = NULL;

    if (!readu2(jcf, &info->number_of_classes))
    {
        jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    if (entry->length != (info->number_of_classes * sizeof(InnerClassInfo) + 2))
    {
        jcf->status = ATTRIBUTE_LENGTH_MISMATCH;
        return 0;
    }

    info->inner_classes = (InnerClassInfo*)malloc(info->number_of_classes * sizeof(InnerClassInfo));

    if (!info->inner_classes)
    {
        jcf->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    uint16_t u16;
    InnerClassInfo* icf = info->inner_classes;

    for (u16 = 0; u16 < info->number_of_classes; u16++, icf++)
    {
        if (!readu2(jcf, &icf->inner_class_index) ||
            !readu2(jcf, &icf->outer_class_index) ||
            !readu2(jcf, &icf->inner_class_name_index) ||
            !readu2(jcf, &icf->inner_class_access_flags))
        {
            jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
            return 0;
        }

        if (icf->inner_class_index == 0 ||
            icf->inner_class_index >= jcf->constantPoolCount ||
            jcf->constantPool[icf->inner_class_index - 1].tag != CONSTANT_Class ||
            icf->outer_class_index >= jcf->constantPoolCount ||
            (icf->outer_class_index > 0 && jcf->constantPool[icf->outer_class_index - 1].tag != CONSTANT_Class) ||
            icf->inner_class_name_index == 0 ||
            icf->inner_class_name_index >= jcf->constantPoolCount ||
            jcf->constantPool[icf->inner_class_name_index - 1].tag != CONSTANT_Utf8)
        {
            jcf->status = ATTRIBUTE_INVALID_INNERCLASS_INDEXES;
            return 0;
        }
    }

    return 1;
}

void printAttributeInnerClasses(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    att_InnerClasses_info* info = (att_InnerClasses_info*)entry->info;
    cp_info* cp;
    char buffer[48];
    uint16_t index;
    InnerClassInfo* innerclass = info->inner_classes;

    ident(identationLevel);
    printf("number_of_classes: %u", info->number_of_classes);

    for (index = 0; index < info->number_of_classes; index++, innerclass++)
    {
        printf("\n\n");
        ident(identationLevel);
        printf("Inner Class #%u:\n", index + 1);

        // inner_class_index
        cp = jcf->constantPool + innerclass->inner_class_index - 1;
        cp = jcf->constantPool + cp->Class.name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
        ident(identationLevel + 1);
        printf("inner_class_info_index:   cp index #%u <%s>\n", innerclass->inner_class_index, buffer);

        // outer_class_index
        ident(identationLevel + 1);
        printf("outer_class_info_index:   cp index #%u ", innerclass->outer_class_index);

        if (innerclass->outer_class_index == 0)
        {
            printf("(no outer class)\n");
        }
        else
        {
            cp = jcf->constantPool + innerclass->outer_class_index - 1;
            cp = jcf->constantPool + cp->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("<%s>\n", buffer);
        }

        // inner_class_name_index
        ident(identationLevel + 1);
        printf("inner_name_index:         cp index #%u ", innerclass->inner_class_name_index);

        if (innerclass->inner_class_name_index == 0)
        {
            printf("(anonymous class)\n");
        }
        else
        {
            cp = jcf->constantPool + innerclass->inner_class_name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("<%s>\n", buffer);
        }

        // inner_class_access_flags
        decodeAccessFlags(innerclass->inner_class_access_flags, buffer, sizeof(buffer), ACCT_INNERCLASS);
        ident(identationLevel + 1);
        printf("inner_class_access_flags: 0x%.4X [%s]", innerclass->inner_class_access_flags, buffer);
    }
}

void freeAttributeInnerClasses(attribute_info* entry)
{
    att_InnerClasses_info* info = (att_InnerClasses_info*)entry->info;

    if (info)
    {
        if (info->inner_classes)
            free(info->inner_classes);

        free(info);
    }

    entry->info = NULL;
}

uint8_t readAttributeLineNumberTable(JavaClassFile* jcf, attribute_info* entry)
{
    att_LineNumberTable_info* info = (att_LineNumberTable_info*)malloc(sizeof(att_LineNumberTable_info));
    entry->info = (void*)info;

    if (!info)
    {
        jcf->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    info->line_number_table = NULL;

    if (!readu2(jcf, &info->line_number_table_length))
    {
        jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    if (entry->length != (info->line_number_table_length * sizeof(LineNumberTableEntry) + 2))
    {
        jcf->status = ATTRIBUTE_LENGTH_MISMATCH;
        return 0;
    }

    info->line_number_table = (LineNumberTableEntry*)malloc(info->line_number_table_length * sizeof(LineNumberTableEntry));

    if (!info->line_number_table)
    {
        jcf->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    uint16_t u16;
    LineNumberTableEntry* lnte = info->line_number_table;

    for (u16 = 0; u16 < info->line_number_table_length; u16++, lnte++)
    {
        if (!readu2(jcf, &lnte->start_pc) ||
            !readu2(jcf, &lnte->line_number))
        {
            jcf->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
            return 0;
        }
    }

    return 1;
}

void printAttributeLineNumberTable(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    att_LineNumberTable_info* info = (att_LineNumberTable_info*)entry->info;
    LineNumberTableEntry* lnte = info->line_number_table;
    uint16_t index;

    ident(identationLevel);
    printf("line_number_table_length: %u", info->line_number_table_length);

    for (index = 0; index < info->line_number_table_length; index++, lnte++)
    {
        printf("\n\n");
        ident(identationLevel);
        printf("Line Number #%u:\n", index + 1);

        ident(identationLevel + 1);
        printf("start_pc:    %u\n", lnte->start_pc);

        ident(identationLevel + 1);
        printf("line_number: %u", lnte->line_number);
    }
}

void freeAttributeLineNumberTable(attribute_info* entry)
{
    att_LineNumberTable_info* info = (att_LineNumberTable_info*)entry->info;

    if (info)
    {
        if (info->line_number_table)
            free(info->line_number_table);

        free(info);
        entry->info = NULL;
    }
}

uint8_t readAttributeCode(JavaClassFile* jcf, attribute_info* entry)
{
    uint32_t length = entry->length;

    while (length-- > 0)
    {
        fgetc(jcf->file);
        jcf->totalBytesRead++;
    }
    return 1;
}

void printAttributeCode(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    ident(identationLevel);
    printf("Not yet implemented.");
}

void freeAttributeCode(attribute_info* entry)
{
    if (entry->info)
    {
        free(entry->info);
        entry->info = NULL;
    }
}

void freeAttributeInfo(attribute_info* entry)
{
    #define ATTR_CASE(attr) case ATTR_##attr: freeAttribute##attr(entry); return;

    switch (entry->attributeType)
    {
        //ATTR_CASE(Code)
        ATTR_CASE(LineNumberTable)
        //ATTR_CASE(SourceFile)
        //ATTR_CASE(InnerClasses)
        //ATTR_CASE(ConstantValue)
        default:
            break;
    }

    #undef ATTR_CASE
}

void printAttribute(JavaClassFile* jcf, attribute_info* entry, int identationLevel)
{
    #define ATTR_CASE(attr) case ATTR_##attr: printAttribute##attr(jcf, entry, identationLevel); return;

    switch (entry->attributeType)
    {
        ATTR_CASE(Code)
        ATTR_CASE(ConstantValue)
        ATTR_CASE(InnerClasses)
        ATTR_CASE(SourceFile)
        ATTR_CASE(LineNumberTable)
        default:
            break;
    }

    ident(identationLevel);
    printf("Attribute not implemented and ignored.");

    #undef ATTR_CASE
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
