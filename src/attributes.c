#include "attributes.h"
#include "readfunctions.h"
#include "utf8.h"
#include "opcodes.h"
#include "debugging.h"
#include <inttypes.h> // Usage of macro "PRId64" to print 64 bit integer

#define DECLARE_ATTR_FUNCS(attr) \
    uint8_t readAttribute##attr(JavaClass* jc, attribute_info* entry); \
    void printAttribute##attr(JavaClass* jc, attribute_info* entry, int identationLevel); \
    void freeAttribute##attr(attribute_info* entry);

DECLARE_ATTR_FUNCS(SourceFile)
DECLARE_ATTR_FUNCS(InnerClasses)
DECLARE_ATTR_FUNCS(LineNumberTable)
DECLARE_ATTR_FUNCS(ConstantValue)
DECLARE_ATTR_FUNCS(Code)
DECLARE_ATTR_FUNCS(Deprecated)
DECLARE_ATTR_FUNCS(Exceptions)

char readAttribute(JavaClass* jc, attribute_info* entry)
{
    entry->info = NULL;

    if (!readu2(jc, &entry->name_index) ||
        !readu4(jc, &entry->length))
    {
        jc->status = UNEXPECTED_EOF;
        return 0;
    }

    // Checks if name_index points to a valid name index in
    // the constant pool
    cp_info* cp = jc->constantPool + entry->name_index - 1;

    if (entry->name_index == 0 ||
        entry->name_index >= jc->constantPoolCount ||
        cp->tag != CONSTANT_Utf8)
    {
        jc->status = INVALID_NAME_INDEX;
        return 0;
    }

    #define IF_ATTR_CHECK(name) \
        if (cmp_UTF8_Ascii(cp->Utf8.bytes, cp->Utf8.length, (uint8_t*)#name, sizeof(#name) - 1)) { \
            entry->attributeType = ATTR_##name; \
            result = readAttribute##name(jc, entry); \
        }

    uint32_t totalBytesRead = jc->totalBytesRead;
    char result;

    IF_ATTR_CHECK(ConstantValue)
    else IF_ATTR_CHECK(SourceFile)
    else IF_ATTR_CHECK(InnerClasses)
    else IF_ATTR_CHECK(Code)
    else IF_ATTR_CHECK(LineNumberTable)
    else IF_ATTR_CHECK(Deprecated)
    else IF_ATTR_CHECK(Exceptions)
    else
    {
        uint32_t u32;

        for (u32 = 0; u32 < entry->length; u32++)
        {
            if (fgetc(jc->file) == EOF)
            {
                jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
                return 0;
            }

            jc->totalBytesRead++;
        }

        result = 1;
        entry->attributeType = ATTR_Unknown;
    }

    if (jc->totalBytesRead - totalBytesRead != entry->length)
    {
        jc->status = ATTRIBUTE_LENGTH_MISMATCH;
        return 0;
    }

    #undef ATTR_CHECK
    return result;
}

void ident(int level)
{
    while (level-- > 0)
        printf("\t");
}

uint8_t readAttributeDeprecated(JavaClass* jc, attribute_info* entry)
{
    entry->info = NULL;
    return 1;
}

void printAttributeDeprecated(JavaClass* jc, attribute_info* entry, int identationLevel)
{
    ident(identationLevel);
    printf("This element is marked as deprecated and should no longer be used.");
}

void freeAttributeDeprecated(attribute_info* entry)
{

}

uint8_t readAttributeConstantValue(JavaClass* jc, attribute_info* entry)
{
    att_ConstantValue_info* info = (att_ConstantValue_info*)malloc(sizeof(att_ConstantValue_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    if (!readu2(jc, &info->constantvalue_index))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    if (info->constantvalue_index == 0 ||
        info->constantvalue_index >= jc->constantPoolCount)
    {
        jc->status = ATTRIBUTE_INVALID_CONSTANTVALUE_INDEX;
        return 0;
    }

    cp_info* cp = jc->constantPool + info->constantvalue_index - 1;

    if (cp->tag != CONSTANT_String && cp->tag != CONSTANT_Float &&
        cp->tag != CONSTANT_Double && cp->tag != CONSTANT_Long &&
        cp->tag != CONSTANT_Integer)
    {
        jc->status = ATTRIBUTE_INVALID_CONSTANTVALUE_INDEX;
        return 0;
    }

    return 1;
}

void printAttributeConstantValue(JavaClass* jc, attribute_info* entry, int identationLevel)
{
    char buffer[48];

    att_ConstantValue_info* info = (att_ConstantValue_info*)entry->info;
    cp_info* cp = jc->constantPool + info->constantvalue_index - 1;

    ident(identationLevel);
    printf("constantvalue_index: #%u <", info->constantvalue_index);

    switch (cp->tag)
    {
        case CONSTANT_Integer:
            printf("%d", (int32_t)cp->Integer.value);
            break;

        case CONSTANT_Long:
            printf("%" PRId64"", ((int64_t)cp->Long.high << 32) | cp->Long.low);
            break;

        case CONSTANT_Float:
            printf("%e", readFloatFromUint32(cp->Float.bytes));
            break;

        case CONSTANT_Double:
            printf("%e", readDoubleFromUint64((uint64_t)cp->Double.high << 32 | cp->Double.low));
            break;

        case CONSTANT_String:
            cp = jc->constantPool + cp->String.string_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("%s", buffer);
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

uint8_t readAttributeSourceFile(JavaClass* jc, attribute_info* entry)
{
    att_SourceFile_info* info = (att_SourceFile_info*)malloc(sizeof(att_SourceFile_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    if (!readu2(jc, &info->sourcefile_index))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    if (info->sourcefile_index == 0 ||
        info->sourcefile_index >= jc->constantPoolCount ||
        jc->constantPool[info->sourcefile_index - 1].tag != CONSTANT_Utf8)
    {
        jc->status = ATTRIBUTE_INVALID_SOURCEFILE_INDEX;
        return 0;
    }

    return 1;
}

void printAttributeSourceFile(JavaClass* jc, attribute_info* entry, int identationLevel)
{
    char buffer[48];
    att_SourceFile_info* info = (att_SourceFile_info*)entry->info;
    cp_info* cp = jc->constantPool + info->sourcefile_index - 1;

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

uint8_t readAttributeInnerClasses(JavaClass* jc, attribute_info* entry)
{
    att_InnerClasses_info* info = (att_InnerClasses_info*)malloc(sizeof(att_InnerClasses_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    info->inner_classes = NULL;

    if (!readu2(jc, &info->number_of_classes))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    info->inner_classes = (InnerClassInfo*)malloc(info->number_of_classes * sizeof(InnerClassInfo));

    if (!info->inner_classes)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    uint16_t u16;
    InnerClassInfo* icf = info->inner_classes;

    for (u16 = 0; u16 < info->number_of_classes; u16++, icf++)
    {
        if (!readu2(jc, &icf->inner_class_index) ||
            !readu2(jc, &icf->outer_class_index) ||
            !readu2(jc, &icf->inner_class_name_index) ||
            !readu2(jc, &icf->inner_class_access_flags))
        {
            jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
            return 0;
        }

        if (icf->inner_class_index == 0 ||
            icf->inner_class_index >= jc->constantPoolCount ||
            jc->constantPool[icf->inner_class_index - 1].tag != CONSTANT_Class ||
            icf->outer_class_index >= jc->constantPoolCount ||
            (icf->outer_class_index > 0 && jc->constantPool[icf->outer_class_index - 1].tag != CONSTANT_Class) ||
            icf->inner_class_name_index >= jc->constantPoolCount ||
            (icf->inner_class_name_index > 0 && jc->constantPool[icf->inner_class_name_index - 1].tag != CONSTANT_Utf8))
        {
            jc->status = ATTRIBUTE_INVALID_INNERCLASS_INDEXES;
            return 0;
        }
    }

    return 1;
}

void printAttributeInnerClasses(JavaClass* jc, attribute_info* entry, int identationLevel)
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
        printf("Inner Class #%u:\n\n", index + 1);

        // inner_class_index
        cp = jc->constantPool + innerclass->inner_class_index - 1;
        cp = jc->constantPool + cp->Class.name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
        ident(identationLevel + 1);
        printf("inner_class_info_index:   #%u <%s>\n", innerclass->inner_class_index, buffer);

        // outer_class_index
        ident(identationLevel + 1);
        printf("outer_class_info_index:   #%u ", innerclass->outer_class_index);

        if (innerclass->outer_class_index == 0)
        {
            printf("(no outer class)\n");
        }
        else
        {
            cp = jc->constantPool + innerclass->outer_class_index - 1;
            cp = jc->constantPool + cp->Class.name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("<%s>\n", buffer);
        }

        // inner_class_name_index
        ident(identationLevel + 1);
        printf("inner_name_index:         #%u ", innerclass->inner_class_name_index);

        if (innerclass->inner_class_name_index == 0)
        {
            printf("(anonymous class)\n");
        }
        else
        {
            cp = jc->constantPool + innerclass->inner_class_name_index - 1;
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

uint8_t readAttributeLineNumberTable(JavaClass* jc, attribute_info* entry)
{
    att_LineNumberTable_info* info = (att_LineNumberTable_info*)malloc(sizeof(att_LineNumberTable_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    info->line_number_table = NULL;

    if (!readu2(jc, &info->line_number_table_length))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    info->line_number_table = (LineNumberTableEntry*)malloc(info->line_number_table_length * sizeof(LineNumberTableEntry));

    if (!info->line_number_table)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    uint16_t u16;
    LineNumberTableEntry* lnte = info->line_number_table;

    for (u16 = 0; u16 < info->line_number_table_length; u16++, lnte++)
    {
        if (!readu2(jc, &lnte->start_pc) ||
            !readu2(jc, &lnte->line_number))
        {
            jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
            return 0;
        }
    }

    return 1;
}

void printAttributeLineNumberTable(JavaClass* jc, attribute_info* entry, int identationLevel)
{
    att_LineNumberTable_info* info = (att_LineNumberTable_info*)entry->info;
    LineNumberTableEntry* lnte = info->line_number_table;
    uint16_t index;

    printf("\n");
    ident(identationLevel);
    printf("line_number_table_length: %u\n\n", info->line_number_table_length);
    ident(identationLevel);
    printf("Table:\tIndex\tline_number\tstart_pc");

    for (index = 0; index < info->line_number_table_length; index++, lnte++)
    {
        printf("\n");
        ident(identationLevel);
        printf("\t%u\t%u\t\t%u", index + 1, lnte->line_number, lnte->start_pc);
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

uint8_t readAttributeCode(JavaClass* jc, attribute_info* entry)
{
    att_Code_info* info = (att_Code_info*)malloc(sizeof(att_Code_info));
    entry->info = (void*)info;
    uint32_t u32;

    if (!info)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    info->code = NULL;
    info->exception_table = NULL;

    if (!readu2(jc, &info->max_stack) ||
        !readu2(jc, &info->max_locals) ||
        !readu4(jc, &info->code_length))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    if (info->code_length == 0 || info->code_length >= 65536)
    {
        jc->status = ATTRIBUTE_INVALID_CODE_LENGTH;
        return 0;
    }

    info->code = (uint8_t*)malloc(info->code_length);

    if (!info->code)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    if (fread(info->code, sizeof(uint8_t), info->code_length, jc->file) != info->code_length)
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    jc->totalBytesRead += info->code_length;

    // TODO: check if all instructions are valid and have correct parameters.

    if (!readu2(jc, &info->exception_table_length))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    info->exception_table = (ExceptionTableEntry*)malloc(info->exception_table_length * sizeof(ExceptionTableEntry));

    if (!info->exception_table)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    ExceptionTableEntry* except = info->exception_table;

    for (u32 = 0; u32 < info->exception_table_length; u32++)
    {
        if (!readu2(jc, &except->start_pc) ||
            !readu2(jc, &except->end_pc) ||
            !readu2(jc, &except->handler_pc) ||
            !readu2(jc, &except->catch_type))
        {
            jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
            return 0;
        }

        // TODO: check if start_pc, end_pc and handler_pc are valid program counters inside
        // the code. Also check if catch_type is a pointer to a valid class with Throwable as
        // parent, or if it is NULL (finally block).
    }

    if (!readu2(jc, &info->attributes_count))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    info->attributes = (attribute_info*)malloc(info->attributes_count * sizeof(attribute_info));

    if (!info->attributes)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    for (u32 = 0; u32 < info->attributes_count; u32++)
    {
        if (!readAttribute(jc, info->attributes + u32))
        {
            // Only "u32" + 1 attributes were attempted to read, so to avoid
            // releasing uninitialized attributes (which could lead to a crash)
            // we set the attributes count to the correct amount
            info->attributes_count = u32 + 1;
            return 0;
        }
    }

    return 1;
}

void printAttributeCode(JavaClass* jc, attribute_info* entry, int identationLevel)
{
    att_Code_info* info = (att_Code_info*)entry->info;
    uint32_t code_offset;

    printf("\n");
    ident(identationLevel);
    printf("max_stack: %u, max_locals: %u, code_length: %u\n", info->max_stack, info->max_locals, info->code_length);
    ident(identationLevel);
    printf("exception_table_length: %u, attribute_count: %u\n\n", info->exception_table_length, info->attributes_count);
    ident(identationLevel);
    printf("Code:\tOffset\tMnemonic\tParameters");

    identationLevel++;

    char buffer[48];
    uint8_t opcode;
    uint32_t u32;
    cp_info* cpi;

    for (code_offset = 0; code_offset < info->code_length; code_offset++)
    {
        opcode = *(info->code + code_offset);

        printf("\n");
        ident(identationLevel);
        printf("%u\t%s", code_offset, getOpcodeMnemonic(opcode));

        #define OPCODE_INTERVAL(begin, end) (opcode >= opcode_##begin && opcode <= opcode_##end)

        // These are all the opcodes that have no parameters
        if (OPCODE_INTERVAL(nop, dconst_1) || OPCODE_INTERVAL(iload_0, saload) ||
            OPCODE_INTERVAL(istore_0, lxor) || OPCODE_INTERVAL(i2l, dcmpg) ||
            OPCODE_INTERVAL(ireturn, return) || OPCODE_INTERVAL(arraylength, athrow) ||
            OPCODE_INTERVAL(monitorenter, monitorexit))
        {
            continue;
        }

        #undef OPCODE_INTERVAL

        #define NEXTBYTE (*(info->code + ++code_offset))

        switch (opcode)
        {
            case opcode_iload:
            case opcode_fload:
            case opcode_dload:
            case opcode_lload:
            case opcode_aload:
            case opcode_istore:
            case opcode_lstore:
            case opcode_fstore:
            case opcode_dstore:
            case opcode_astore:
            case opcode_ret:

                printf("\t\t%u", NEXTBYTE);
                break;

            case opcode_newarray:

                u32 = NEXTBYTE;
                printf("\t%u (array of %s)", u32, decodeOpcodeNewarrayType((uint8_t)u32));
                break;

            case opcode_bipush:

                printf("\t\t%d", (int8_t)NEXTBYTE);
                break;

            case opcode_getfield:
            case opcode_getstatic:
            case opcode_putfield:
            case opcode_putstatic:
            case opcode_invokevirtual:
            case opcode_invokespecial:
            case opcode_invokestatic:

                u32 = NEXTBYTE;
                u32 = (u32 << 8) | NEXTBYTE;

                printf("\t#%u ", u32);
                cpi = jc->constantPool + u32 - 1;

                // Invokes require a Methodref a parameter, whereas getfield, getstatic,
                // putfield and putstatic require a Fieldref.

                if ((opcode <  opcode_invokevirtual && cpi->tag == CONSTANT_Fieldref) ||
                    (opcode >= opcode_invokevirtual && cpi->tag == CONSTANT_Methodref))
                {
                    cpi = jc->constantPool + cpi->Fieldref.class_index - 1;
                    cpi = jc->constantPool + cpi->Class.name_index - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("(%s %s.", opcode < opcode_invokevirtual ? "Field" : "Method", buffer);

                    cpi = jc->constantPool + u32 - 1;
                    cpi = jc->constantPool + cpi->Fieldref.name_and_type_index - 1;
                    u32 = cpi->NameAndType.descriptor_index;
                    cpi = jc->constantPool + cpi->NameAndType.name_index - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s, descriptor: ", buffer);

                    cpi = jc->constantPool + u32 - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s)", buffer);
                }
                else
                {
                    printf("(%s, invalid - not a %s)", decodeTag(cpi->tag), opcode < opcode_invokevirtual ? "Field" : "Method");
                }

                break;

            case opcode_invokedynamic:

                u32 = NEXTBYTE;
                u32 = (u32 << 8) | NEXTBYTE;

                printf("\t#%u - CONSTANT_InvokeDynamic not implemented -", u32);

                u32 = NEXTBYTE;

                if (u32 != 0)
                {
                    printf("\n");
                    ident(identationLevel);
                    printf("%u\t- expected a zero byte in this offset due to invokedynamic, found 0x%.2X instead -", code_offset, u32);
                }

                u32 = NEXTBYTE;

                if (u32 != 0)
                {
                    printf("\n");
                    ident(identationLevel);
                    printf("%u\t- expected a zero byte in this offset due to invokedynamic, found 0x%.2X instead -", code_offset, u32);
                }

                break;

            case opcode_invokeinterface:

                u32 = NEXTBYTE;
                u32 = (u32 << 8) | NEXTBYTE;

                printf("\t#%u, count: %u ", u32, NEXTBYTE);

                cpi = jc->constantPool + u32 - 1;

                if (cpi->tag == CONSTANT_InterfaceMethodref)
                {
                    cpi = jc->constantPool + cpi->InterfaceMethodref.class_index - 1;
                    cpi = jc->constantPool + cpi->Class.name_index - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("(InterfaceMethod %s.", buffer);

                    cpi = jc->constantPool + u32 - 1;
                    cpi = jc->constantPool + cpi->InterfaceMethodref.name_and_type_index - 1;
                    u32 = cpi->NameAndType.descriptor_index;
                    cpi = jc->constantPool + cpi->NameAndType.name_index - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s, descriptor: ", buffer);

                    cpi = jc->constantPool + u32 - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s)", buffer);
                }
                else
                {
                    printf("(%s, invalid - not a InterfaceMethod)", decodeTag(cpi->tag));
                }

                u32 = NEXTBYTE;

                if (u32 != 0)
                {
                    printf("\n");
                    ident(identationLevel);
                    printf("%u\t- expected a zero byte in this offset due to invokeinterface, found 0x%.2X instead -", code_offset, u32);
                }

                break;

            case opcode_ifeq:
            case opcode_ifne:
            case opcode_iflt:
            case opcode_ifge:
            case opcode_ifgt:
            case opcode_ifle:
            case opcode_goto:
            case opcode_jsr:
            case opcode_sipush:
            case opcode_ifnull:

                printf("\t");

                // No break here, since the parameter handling of the above opcodes
                // is the same as the opcodes below. The difference is the need
                // of another '\t' to keep the printing neat.

            case opcode_if_icmpeq:
            case opcode_if_icmpne:
            case opcode_if_icmplt:
            case opcode_if_icmpge:
            case opcode_if_icmpgt:
            case opcode_if_icmple:
            case opcode_if_acmpeq:
            case opcode_if_acmpne:
            case opcode_ifnonnull:

                u32 = (uint16_t)NEXTBYTE << 8;
                u32 |= NEXTBYTE;

                printf("\tpc + %d = address %d", (int16_t)u32, (int16_t)u32 + code_offset - 2);
                break;

            case opcode_goto_w:
            case opcode_jsr_w:

                u32 = NEXTBYTE;
                u32 = (u32 << 8) | NEXTBYTE;
                u32 = (u32 << 8) | NEXTBYTE;
                u32 = (u32 << 8) | NEXTBYTE;
                printf("\t\t%d", (int32_t)u32);
                break;

            case opcode_iinc:

                printf("\t\t%u,", NEXTBYTE);
                printf(" %d", (int8_t)NEXTBYTE);
                break;

            case opcode_new:
            case opcode_anewarray:
            case opcode_checkcast:
            case opcode_instanceof:
            case opcode_multianewarray:

                u32 = NEXTBYTE;
                u32 = (u32 << 8) | NEXTBYTE;

                if (opcode == opcode_new)
                    printf("\t");

                printf("\t#%u", u32);

                if (opcode == opcode_multianewarray)
                    printf(", dimension %u", NEXTBYTE);

                cpi = jc->constantPool + u32 - 1;

                if (cpi->tag == CONSTANT_Class)
                {
                    cpi = jc->constantPool + cpi->Class.name_index - 1;
                    UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf(" (class: %s)", buffer);
                }
                else
                {
                    printf(" (%s, invalid - not a class)", decodeTag(cpi->tag));
                }

                break;

            case opcode_ldc:
            case opcode_ldc_w:
            case opcode_ldc2_w:

                u32 = NEXTBYTE;

                if (opcode == opcode_ldc_w || opcode == opcode_ldc2_w)
                    u32 = (u32 << 8) | NEXTBYTE;

                printf("\t\t#%u", u32);

                cpi = jc->constantPool + u32 - 1;

                if (opcode == opcode_ldc2_w)
                {
                    if (cpi->tag == CONSTANT_Long)
                        printf(" (long: %" PRId64")", ((int64_t)cpi->Long.high << 32) | cpi->Long.low);
                    else if (cpi->tag == CONSTANT_Double)
                        printf(" (double: %e)", readDoubleFromUint64((uint64_t)cpi->Double.high << 32 | cpi->Double.low));
                    else
                        printf(" (%s, invalid)", decodeOpcodeNewarrayType(cpi->tag));
                }
                else
                {
                    if (cpi->tag == CONSTANT_Class)
                    {
                        cpi = jc->constantPool + cpi->Class.name_index - 1;
                        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                        printf(" (class: %s)", buffer);
                    }
                    else if (cpi->tag == CONSTANT_String)
                    {
                        cpi = jc->constantPool + cpi->String.string_index - 1;
                        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                        printf(" (string: %s)", buffer);
                    }
                    else if (cpi->tag == CONSTANT_Integer)
                    {
                        printf(" (integer: %d)", (int32_t)cpi->Integer.value);
                    }
                    else if (cpi->tag == CONSTANT_Float)
                    {
                        printf(" (float: %e)", readFloatFromUint32(cpi->Float.bytes));
                    }
                    else
                    {
                        printf(" (%s, invalid)", decodeTag(cpi->tag));
                    }
                }

                break;

            case opcode_wide:

                opcode = NEXTBYTE;

                switch (opcode)
                {
                    case opcode_iload:
                    case opcode_fload:
                    case opcode_dload:
                    case opcode_lload:
                    case opcode_aload:
                    case opcode_istore:
                    case opcode_lstore:
                    case opcode_fstore:
                    case opcode_dstore:
                    case opcode_astore:
                    case opcode_ret:

                        u32 = NEXTBYTE;
                        u32 = (u32 << 8) | NEXTBYTE;

                        printf(" %s\t%u", getOpcodeMnemonic(opcode), u32);

                        break;

                    case opcode_iinc:

                        u32 = NEXTBYTE;
                        u32 = (u32 << 8) | NEXTBYTE;

                        printf(" iinc\t%u,", u32);

                        u32 = NEXTBYTE;
                        u32 = (u32 << 8) | NEXTBYTE;

                        printf(" %d", (int16_t)u32);

                        break;

                    default:
                        printf(" %s (invalid following instruction, can't continue)", getOpcodeMnemonic(opcode));
                        code_offset = info->code_length;
                        break;
                }

                break;

            case opcode_tableswitch:
            {
                uint32_t base_address = code_offset;

                while ((code_offset + 1) % 4)
                    u32 = NEXTBYTE;

                int32_t defaultValue = NEXTBYTE;
                defaultValue = (defaultValue << 8) | NEXTBYTE;
                defaultValue = (defaultValue << 8) | NEXTBYTE;
                defaultValue = (defaultValue << 8) | NEXTBYTE;

                int32_t lowValue = NEXTBYTE;
                lowValue = (lowValue << 8) | NEXTBYTE;
                lowValue = (lowValue << 8) | NEXTBYTE;
                lowValue = (lowValue << 8) | NEXTBYTE;

                int32_t highValue = NEXTBYTE;
                highValue = (highValue << 8) | NEXTBYTE;
                highValue = (highValue << 8) | NEXTBYTE;
                highValue = (highValue << 8) | NEXTBYTE;

                if (lowValue > highValue)
                {
                    printf("\tinvalid operands - lowValue (%d) is greater than highValue (%d)\n", lowValue, highValue);
                    ident(identationLevel);
                    printf("- can't continue -");
                    code_offset = info->code_length;
                    break;
                }
                else
                {
                    printf("\tlow = %d, high = %d", lowValue, highValue);
                }

                int32_t offset;

                for (u32 = 0; u32 < (uint32_t)(highValue - lowValue + 1); u32++)
                {
                    printf("\n");
                    ident(identationLevel);
                    printf("%u\t", code_offset);

                    offset = NEXTBYTE;
                    offset = (offset << 8) | NEXTBYTE;
                    offset = (offset << 8) | NEXTBYTE;
                    offset = (offset << 8) | NEXTBYTE;

                    printf("  case %d: pc + %d = address %d", lowValue + u32, offset, offset + base_address);
                }

                printf("\n");
                ident(identationLevel);
                printf("-\t  default: pc + %d = address %d", defaultValue, defaultValue + base_address);

                break;
            }

            case opcode_lookupswitch:
            {
                uint32_t base_address = code_offset;

                while ((code_offset + 1) % 4)
                    u32 = NEXTBYTE;

                int32_t defaultValue = NEXTBYTE;
                defaultValue = (defaultValue << 8) | NEXTBYTE;
                defaultValue = (defaultValue << 8) | NEXTBYTE;
                defaultValue = (defaultValue << 8) | NEXTBYTE;

                int32_t npairs = NEXTBYTE;
                npairs = (npairs << 8) | NEXTBYTE;
                npairs = (npairs << 8) | NEXTBYTE;
                npairs = (npairs << 8) | NEXTBYTE;

                if (npairs < 0)
                {
                    printf("\tinvalid operand - npairs (%d) should be greater than or equal to 0\n", npairs);
                    ident(identationLevel);
                    printf("- can't continue -");
                    code_offset = info->code_length;
                    break;
                }
                else
                {
                    printf("\tnpairs = %d", npairs);
                }

                int32_t match, offset;

                while (npairs-- > 0)
                {
                    printf("\n");
                    ident(identationLevel);
                    printf("%u\t", code_offset);

                    match = NEXTBYTE;
                    match = (match << 8) | NEXTBYTE;
                    match = (match << 8) | NEXTBYTE;
                    match = (match << 8) | NEXTBYTE;

                    offset = NEXTBYTE;
                    offset = (offset << 8) | NEXTBYTE;
                    offset = (offset << 8) | NEXTBYTE;
                    offset = (offset << 8) | NEXTBYTE;

                    printf("  case %d: pc + %d = address %d", match, offset, offset + base_address);
                }

                printf("\n");
                ident(identationLevel);
                printf("-\t  default: pc + %d = address %d", defaultValue, defaultValue + base_address);

                break;
            }


            default:
                printf("\n");
                ident(identationLevel);
                printf("- last instruction was not recognized, can't continue -");
                // To stop the for loop, as not knowing the opcode could lead to wrong interpretation of bytes
                code_offset = info->code_length;
                break;
        }
    }

    printf("\n");
    identationLevel--;

    if (info->exception_table_length > 0)
    {
        printf("\n");
        ident(identationLevel);
        printf("Exception Table:\n");
        ident(identationLevel);
        printf("Index\tstart_pc\tend_pc\thandler_pc\tcatch_type");

        ExceptionTableEntry* except = info->exception_table;

        for (u32 = 0; u32 < info->exception_table_length; u32++)
        {
            printf("\n");
            ident(identationLevel);
            printf("%u\t%u\t\t%u\t%u\t\t%u", u32 + 1, except->start_pc, except->end_pc, except->handler_pc, except->catch_type);

            if (except->catch_type > 0)
            {
                cpi = jc->constantPool + except->catch_type - 1;
                cpi = jc->constantPool + cpi->Class.name_index - 1;
                UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                printf(" <%s>", buffer);
            }
        }

        printf("\n");
    }

    if (info->attributes_count > 0)
    {
        for (u32 = 0; u32 < info->attributes_count; u32++)
        {
            attribute_info* atti = info->attributes + u32;
            cpi = jc->constantPool + atti->name_index - 1;
            UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

            printf("\n");
            ident(identationLevel);
            printf("Code Attribute #%u - %s:\n", u32 + 1, buffer);
            printAttribute(jc, atti, identationLevel + 1);
        }
    }
}

void freeAttributeCode(attribute_info* entry)
{
    att_Code_info* info = (att_Code_info*)entry->info;

    if (info)
    {
        if (info->code)
            free(info->code);

        if (info->exception_table)
            free(info->exception_table);

        if (info->attributes)
        {
            uint16_t u16;

            for (u16 = 0; u16 < info->attributes_count; u16++)
                freeAttributeInfo(info->attributes + u16);

            free(info->attributes);
        }

        free(info);
        entry->info = NULL;
    }
}

uint8_t readAttributeExceptions(JavaClass* jc, attribute_info* entry)
{
    att_Exceptions_info* info = (att_Exceptions_info*)malloc(sizeof(att_Exceptions_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    info->exception_index_table = NULL;

    if (!readu2(jc, &info->number_of_exceptions))
    {
        jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
        return 0;
    }

    info->exception_index_table = (uint16_t*)malloc(info->number_of_exceptions * sizeof(uint16_t));

    if (!info->exception_index_table)
    {
        jc->status = MEMORY_ALLOCATION_FAILED;
        return 0;
    }

    uint16_t u16;
    uint16_t* exception_index = info->exception_index_table;

    for (u16 = 0; u16 < info->number_of_exceptions; u16++, exception_index++)
    {
        if (!readu2(jc, exception_index))
        {
            jc->status = UNEXPECTED_EOF_READING_ATTRIBUTE_INFO;
            return 0;
        }

        if (*exception_index == 0 ||
            *exception_index >= jc->constantPoolCount ||
            jc->constantPool[*exception_index - 1].tag != CONSTANT_Class)
        {
            jc->status = ATTRIBUTE_INVALID_EXCEPTIONS_CLASS_INDEX;
            return 0;
        }
    }

    return 1;
}

void printAttributeExceptions(JavaClass* jc, attribute_info* entry, int identationLevel)
{
    att_Exceptions_info* info = (att_Exceptions_info*)entry->info;
    uint16_t* exception_index = info->exception_index_table;
    uint16_t index;
    char buffer[48];
    cp_info* cpi;

    printf("\n");
    ident(identationLevel);
    printf("number_of_exceptions: %u", info->number_of_exceptions);

    for (index = 0; index < info->number_of_exceptions; index++, exception_index++)
    {
        cpi = jc->constantPool + *exception_index - 1;
        cpi = jc->constantPool + cpi->Class.name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

        printf("\n\n");
        ident(identationLevel + 1);
        printf("Exception #%u: #%u <%s>\n", index + 1, *exception_index, buffer);
    }
}

void freeAttributeExceptions(attribute_info* entry)
{
    att_Exceptions_info* info = (att_Exceptions_info*)entry->info;

    if (info)
    {
        if (info->exception_index_table)
            free(info->exception_index_table);

        free(info);
        entry->info = NULL;
    }
}

void freeAttributeInfo(attribute_info* entry)
{
    #define ATTR_CASE(attr) case ATTR_##attr: freeAttribute##attr(entry); return;

    switch (entry->attributeType)
    {
        ATTR_CASE(Code)
        ATTR_CASE(LineNumberTable)
        ATTR_CASE(SourceFile)
        ATTR_CASE(InnerClasses)
        ATTR_CASE(ConstantValue)
        ATTR_CASE(Deprecated)
        ATTR_CASE(Exceptions)
        default:
            break;
    }

    #undef ATTR_CASE
}

void printAttribute(JavaClass* jc, attribute_info* entry, int identationLevel)
{
    #define ATTR_CASE(attr) case ATTR_##attr: printAttribute##attr(jc, entry, identationLevel); break;

    switch (entry->attributeType)
    {
        ATTR_CASE(Code)
        ATTR_CASE(ConstantValue)
        ATTR_CASE(InnerClasses)
        ATTR_CASE(SourceFile)
        ATTR_CASE(LineNumberTable)
        ATTR_CASE(Deprecated)
        ATTR_CASE(Exceptions)
        default:
            ident(identationLevel);
            printf("Attribute not implemented and ignored.");
            break;
    }

    printf("\n");

    #undef ATTR_CASE
}

void printAllAttributes(JavaClass* jc)
{
    if (jc->attributeCount == 0)
        return;

    uint16_t u16;
    char buffer[48];
    cp_info* cp;
    attribute_info* atti;

    printf("\n---- Class Attributes ----");

    for (u16 = 0; u16 < jc->attributeCount; u16++)
    {
        atti = jc->attributes + u16;
        cp = jc->constantPool + atti->name_index - 1;
        UTF8_to_Ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);

        printf("\n\n\tAttribute #%u - %s:\n\n", u16 + 1, buffer);
        printAttribute(jc, atti, 2);
    }
}

attribute_info* getAttributeByType(attribute_info* attributes, uint16_t attributes_length, enum AttributeType type)
{
    while (attributes_length-- > 0)
    {
        if (attributes->attributeType == type)
            return attributes;

        attributes++;
    }

    return NULL;
}
