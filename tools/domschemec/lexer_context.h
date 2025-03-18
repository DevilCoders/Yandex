#pragma once

#include "pragma.h"

#include <util/stream/file.h>
#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/stack.h>
#include <util/generic/yexception.h>

namespace NDomSchemeCompiler {

struct TPosition {
    TString FileName;
    size_t Line;
    size_t Offset;
};

struct IFileResolver {
    virtual TSimpleSharedPtr<IFileResolver> Resolve(const TString&) = 0;
    virtual const TString ReadAll() = 0;
    virtual const TString& FileName() = 0;
    virtual ~IFileResolver(){};
};

class TDefaultFileResolver: public IFileResolver {
public:
    TDefaultFileResolver(const TString& currentFileName)
        : CurrentFileName(currentFileName)
    {
    }

    TSimpleSharedPtr<IFileResolver> Resolve(const TString& includeFile) {
        TString includeFileName(JoinFsPaths(TFsPath(CurrentFileName).Dirname(), includeFile));
        if (!TFsPath(includeFileName).IsFile()) {
            ythrow yexception() << "Cannot find file: " << includeFile;
        }
        return MakeSimpleShared<TDefaultFileResolver>(includeFileName);
    }
    const TString ReadAll() {
        return TFileInput(CurrentFileName).ReadAll();
    }
    const TString& FileName() {
        return CurrentFileName;
    }

private:
    TString CurrentFileName;
};

struct ILexerContext {
    virtual void Push(const TString& file) = 0;
    virtual bool Pop() = 0;
    virtual const TPragmas& GetPragmas() const = 0;
    virtual TPragmas& SetPragmas() = 0;
    virtual TPosition& Position() = 0;
    virtual TString& Code() = 0;
    virtual const char*& CurChar() = 0;
    virtual TStringBuf& NextToken() = 0;
    virtual ~ILexerContext(){};
};

class TDefaultLexerContext: public ILexerContext {
    struct TCtx {
        TSimpleSharedPtr<IFileResolver> Resolver;
        TString Code;
        const char* CurChar;
        TStringBuf NextToken;
        TPosition Position;
        TPragmas Pragmas;
        TCtx(TSimpleSharedPtr<IFileResolver> resolver)
            : Resolver(resolver)
            , Code(Resolver->ReadAll())
            , CurChar(Code.data())
            , Position({Resolver->FileName(), 0, 0})
        {
        }
    };
    using TCtxStack = TStack<TCtx>;

public:
    TDefaultLexerContext(TSimpleSharedPtr<IFileResolver> resolver) {
        CtxStack.emplace(resolver);
    }
    TDefaultLexerContext(const TString& file) {
        CtxStack.emplace(MakeSimpleShared<TDefaultFileResolver>(file));
    }
    void Push(const TString& file) {
        CtxStack.emplace(CtxStack.top().Resolver->Resolve(file));
    }
    const TPragmas& GetPragmas() const {
        return CtxStack.top().Pragmas;
    }
    TPragmas& SetPragmas() {
        return CtxStack.top().Pragmas;
    }
    bool Pop() {
        CtxStack.pop();
        return !CtxStack.empty();
    }
    TPosition& Position() {
        return CtxStack.top().Position;
    }
    TString& Code() {
        return CtxStack.top().Code;
    }
    const char*& CurChar() {
        return CtxStack.top().CurChar;
    }
    TStringBuf& NextToken() {
        return CtxStack.top().NextToken;
    }
private:
    TCtxStack CtxStack;
};

}
