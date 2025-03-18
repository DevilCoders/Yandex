#pragma once

#include "node.h"

#include <util/memory/pool.h>

namespace NHtml5 {
    /** The output struct containing the results of the parse. */
    class TOutput {
    public:
        /**
     * Pointer to the document node.  This is a TNode of type NODE_DOCUMENT
     * that contains the entire document as its child.
     */
        TNode* Document;

        /**
     * Pointer to the root node.  This the <html> tag that forms the root of the
     * document.
     */
        TNode* Root;

        /**
     * A list of errors that occurred during the parse.
     * NOTE: In version 1.0 of this library, the API for errors hasn't been fully
     * fleshed out and may change in the future.  For this reason, the GumboError
     * header isn't part of the public API.  Contact us if you need errors
     * reported so we can work out something appropriate for your use-case.
     */
        //GumboVector /* GumboError */ errors;

    public:
        TOutput(size_t bufsize = 64 << 10);
        ~TOutput();

        TNode* CreateNode(const ENodeType type);

        TStringPiece CreateString(const TStringPiece& str);
        TStringPiece CreateString(const char* str, size_t len);
        TStringPiece CreateString(const char* str);
        TStringPiece ConcatString(const TStringPiece& a, const TStringPiece& b);

        template <typename T>
        T* CreateVector(size_t len) {
            return Memory_.AllocateArray<T>(len);
        }

    private:
        TMemoryPool Memory_;
    };

}
