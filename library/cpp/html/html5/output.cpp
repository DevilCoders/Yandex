#include "output.h"

namespace NHtml5 {
    TOutput::TOutput(size_t bufsize)
        : Memory_(bufsize, TMemoryPool::TLinearGrow::Instance())
    {
    }

    TOutput::~TOutput() {
    }

    TNode* TOutput::CreateNode(const ENodeType type) {
        TNode* const node = Memory_.New<TNode>();
        node->Type = type;
        return node;
    }

    TStringPiece TOutput::ConcatString(const TStringPiece& a, const TStringPiece& b) {
        TStringPiece ret;
        ret.Length = a.Length + b.Length;
        ret.Data = Memory_.AllocateArray<char>(ret.Length);
        memcpy((void*)(ret.Data), a.Data, a.Length);
        memcpy((void*)(ret.Data + a.Length), b.Data, b.Length);
        return ret;
    }

    TStringPiece TOutput::CreateString(const char* str, size_t len) {
        TStringPiece ret;
        ret.Data = str;
        ret.Length = len;
        return CreateString(ret);
    }

    TStringPiece TOutput::CreateString(const TStringPiece& str) {
        if (!str.Data || str.Length == 0) {
            return TStringPiece::Empty();
        }
        TStringPiece ret;
        ret.Data = Memory_.AllocateArray<char>(str.Length);
        ret.Length = str.Length;
        memcpy((void*)ret.Data, str.Data, str.Length);
        return ret;
    }

    TStringPiece TOutput::CreateString(const char* str) {
        return CreateString(str, strlen(str));
    }

}
