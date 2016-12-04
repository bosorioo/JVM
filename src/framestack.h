#ifndef FRAMESTACK_H
#define FRAMESTACK_H

typedef struct Frame Frame;
typedef struct FrameStack FrameStack;

#include "operandstack.h"
#include "attributes.h"

/// @brief Structure that will be associated with a method and
/// holds the information necessary to run the method.
struct Frame
{
    /// @brief Class of the method associated with this frame.
    JavaClass* jc;

    /// @brief Number of operands that should be moved from this frame to
    /// the caller frame when the method returns.
    uint8_t returnCount;

    /// @brief Index of the current bytecode being read from the
    /// method's instructions, Program Counter.
    uint32_t pc;

    /// @brief Number of bytes the bytecode of the method contains.
    uint32_t code_length;

    /// @brief Bytecode of the method.
    uint8_t* code;

    /// @brief Stack of operands used by the method.
    OperandStack* operands;

    /// @brief Array of local variables used by the method.
    int32_t* localVariables;

#ifdef DEBUG
    uint16_t max_locals;
#endif
};

/// @brief Stack of frames to store all frames created
/// by method calling during execution of the JVM.
struct FrameStack
{
    /// @brief The node value of the stack.
    Frame* frame;

    /// @brief Pointer to the next element of the stack.
    struct FrameStack* next;
};

Frame* newFrame(JavaClass* jc, method_info* method);
void freeFrame(Frame* frame);
uint8_t pushFrame(FrameStack** fs, Frame* frame);
uint8_t popFrame(FrameStack** fs, Frame* outPtr);
void freeFrameStack(FrameStack** fs);

#endif // FRAMESTACK_H
