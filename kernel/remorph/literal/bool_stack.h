#pragma once

#include <util/generic/bitmap.h>
#include <util/generic/vector.h>

// Use STL-based implementation in DEBUG in order to validate number of states
#if !defined(NDEBUG)
    #define STACK_DECLARE(name)         TVector<bool> name
    #define STACK_INVERT(name)          name.back() = !name.back()
    #define STACK_JOIN(name, op)        {const bool v = name.back(); name.pop_back(); name.back() = v op name.back();}
    #define STACK_XOR(name)             {const bool v = name.back(); name.pop_back(); name.back() = (!v == name.back());}
    #define STACK_PUSH(name, val)       name.push_back(val)
    #define STACK_TOP(name)             name.back()
    #define STACK_VALIDATE(name)        Y_ASSERT(1 == name.size())
#else
// Use fast bitmap in release
    #define STACK_DECLARE(name)         TDynBitMap name
    #define STACK_INVERT(name)          name[0].Flip()
    #define STACK_JOIN(name, op)        {const bool v = name.Pop(); name[0] = v op name[0];}
    #define STACK_XOR(name)             {const bool v = name.Pop(); name[0] = (!v == name[0]);}
    #define STACK_PUSH(name, val)       name.Push(val)
    #define STACK_TOP(name)             name.Get(0)
    #define STACK_VALIDATE(name)
#endif
