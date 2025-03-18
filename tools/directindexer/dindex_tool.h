#pragma once

#include <util/generic/ptr.h>
#include <kernel/groupattrs/creator/creator.h>
#include <kernel/indexer/directindex/directindex.h>

enum ECommand {
    C_UNKNOWN,
    C_CREATE_SERVER,
    C_CLOSE_SERVER,
    C_ADD_DOC,
    C_COMMIT_DOC,
    C_ADD_ATTRIBUTES,
    C_OPEN_ZONE,
    C_CLOSE_ZONE,
    C_ADD_TEXT,
    C_INC_BREAK,
    C_NEXT_WORD,
    C_SET
};

class TDindexServer {
public:
    TDindexServer(const TString& workDir, const TString& prefix, size_t docCount, const TString& attrsNames);
    ~TDindexServer();

    void CreateIndex();
    void Close();

    static ECommand GetCommand(const TString& command);
    void EvalCommand(ECommand command, const TVector<TString>& tokens);

private:
    const TString Name; // Name of archive
    TString ResultDir;
    TString WorkDir;
    TString Prefix;
    size_t DocCount;
    struct TImpl;
    THolder<TImpl> Impl;
    THolder<NGroupingAttrs::TCreator> AttrCreator;
    TVector<TString> AttrsNames;

    NIndexerCore::TDirectIndex::TWAttributes DocAttributes;
    ui32 DocId; // identifier of commited document

    static NIndexerCore::TDirectIndex::TWAttributes ParseAttributes(const TVector<TString>& tokens, int firstPos);
};
