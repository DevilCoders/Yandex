#pragma once

#include <tools/domschemec/lexer_context.h>
#include <tools/domschemec/ast.h>
#include <tools/domschemec/lexer.h>
#include <tools/domschemec/parser.h>
#include <tools/domschemec/generate.h>

#include <util/stream/str.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>

#include <memory>

class TVFSFileResolver : public NDomSchemeCompiler::IFileResolver {
public:
    using TVFS = TVector<std::pair<TString, TString>>;
    TVFSFileResolver(const TVFS& vfs, const TString& fname) : VFS(vfs), CurrentFileName(fname) {}

    const TString* FindFile(const TString& name) {
        for(const auto& [fname, contents] : VFS) {
            if (fname.equal(name)) {
                return &contents;
            }
        }
        return nullptr;
    }

    TSimpleSharedPtr<IFileResolver> Resolve(const TString& includeFile) {
        TString includeFileName(JoinFsPaths(TFsPath(CurrentFileName).Dirname(), includeFile));
        if (!FindFile(includeFileName)) {
            ythrow yexception() << "Cannot read file: " << includeFile;
        }
        return MakeSimpleShared<TVFSFileResolver>(VFS, includeFileName);
    }

    const TString ReadAll() {
        return *FindFile(CurrentFileName);
    }

    const TString& FileName() {
        return CurrentFileName;
    }

private:
    const TVFS& VFS;
    TString CurrentFileName;
};

static inline TString Process(const TVFSFileResolver::TVFS& vfs, const TString& file)
{
    auto resolver = MakeSimpleShared<TVFSFileResolver>(vfs, file);
    auto lexerContext = MakeHolder<NDomSchemeCompiler::TDefaultLexerContext>(resolver);
    NDomSchemeCompiler::TParser p(std::move(lexerContext));
    p.Run();
    TString result;
    TStringOutput out(result);
    NDomSchemeCompiler::Generate(p.GetProgram(), out);
    return result;
}

