#include "framestack.h"
#include "debugging.h"

Frame* newFrame(JavaClass* jc, method_info* method)
{
    Frame* frame = (Frame*)malloc(sizeof(Frame));

    if (frame)
    {
        attribute_info* codeAttribute = getAttributeByType(method->attributes, method->attributes_count, ATTR_Code);
        att_Code_info* code;

        if (codeAttribute)
        {
            code = (att_Code_info*)codeAttribute->info;
            frame->code = code->code;
            frame->code_length = code->code_length;

            if (code->max_locals > 0)
                frame->localVariables = (int32_t*)malloc(code->max_locals * sizeof(int32_t));
            else
                frame->localVariables = NULL;

#ifdef DEBUG
            frame->max_locals = code->max_locals;
#endif // DEBUG

        }
        else
        {
            frame->code = NULL;
            frame->code_length = 0;
            frame->localVariables = NULL;
        }

        frame->operands = NULL;
        frame->jc = jc;
        frame->pc = 0;
        frame->fp_strict = (method->access_flags & ACC_STRICT) != 0;
    }

    return frame;
}

void freeFrame(Frame* frame)
{
    if (frame->localVariables)
        free(frame->localVariables);

    if (frame->operands)
        freeOperandStack(&frame->operands);

    free(frame);
}

uint8_t pushFrame(FrameStack** fs, Frame* frame)
{
    FrameStack* node = (FrameStack*)malloc(sizeof(FrameStack));

    if (node)
    {
        node->frame = frame;
        node->next = *fs;
        *fs = node;
    }

    return fs != NULL;
}

uint8_t popFrame(FrameStack** fs, Frame* outPtr)
{
    FrameStack* node = *fs;

    if (node)
    {
        if (outPtr)
            *outPtr = *node->frame;

        *fs = node->next;
        free(node);
    }

    return node != NULL;
}

void freeFrameStack(FrameStack** fs)
{
    FrameStack* node = *fs;
    FrameStack* tmp;

    while (node)
    {
        tmp = node;
        node = node->next;
        freeFrame(tmp->frame);
        free(tmp);
    }

    *fs = NULL;
}
