#pragma once

#include "decl.h"

#include <library/cpp/numerator/numerate.h>

namespace NDomTree {
    class IPositionProvider {
    public:
        virtual void Position(size_t offset, ui64& line, ui64& column) const = 0;
        virtual TString GetContext(ui64 line, ui64 column, TMaybe<size_t>& position) const = 0;
    };

    class ITreeBuilderHandler: public INumeratorHandler {
    public:
        virtual TDomTreePtr GetTree() const = 0;
        virtual size_t GetMaxDepth() const = 0;
        virtual void SetPositionProvider(IPositionProvider* /*positionProvider*/) {
        }
        virtual void SetParsedDocument(TStringBuf /*document*/) {
        }
        virtual void SetDecodeHtEntity(bool /*decode*/) {
        }
        virtual void SetProcessScriptText(bool /*processScriptText*/) {
        }
    };

    using TNumHandlerPtr = TSimpleSharedPtr<ITreeBuilderHandler>;
    TNumHandlerPtr TreeBuildingHandler();
}
