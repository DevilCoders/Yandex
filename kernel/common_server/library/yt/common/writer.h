#pragma once

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/node.h>
#include <mapreduce/yt/interface/io-inl.h>
#include <mapreduce/yt/interface/init.h>

struct TTableSelectorDay {
public:
    static TString GetTable(const TInstant& ts) {
        return TString(ts.ToString().data(), 10);
    }
};

struct TTableSelectorMonth {
public:
    static TString GetTable(const TInstant& ts) {
        return TString(ts.ToString().data(), 7);
    }
};

class IYTWritersSet {
public:
    virtual ~IYTWritersSet() {}
    virtual void Finish() = 0;
    virtual NYT::TTableWriterPtr<NYT::TNode> GetWriter(const TInstant& ts) = 0;
};

template <class TTableSelector = TTableSelectorDay>
class TYTWritersSet : public IYTWritersSet {
public:
    TYTWritersSet(NYT::IClientPtr ytClient, const NYT::TYPath& workDir, const NYT::TTableSchema& schema = {}, const bool optimizeForScan = false)
        : Client(ytClient)
        , Transaction(ytClient ? ytClient->StartTransaction() : nullptr)
        , WorkDir(workDir)
        , Schema(schema)
        , OptimizeForScan(optimizeForScan)
    {}

    void Finish() {
        for (auto&& writer : Writers) {
            writer.second->Finish();
        }
        if (Transaction) {
            Transaction->Commit();
        }
    }

    NYT::TTableWriterPtr<NYT::TNode> GetWriter(const TInstant& ts) {
        TString path = TTableSelector::GetTable(ts);
        auto it = Writers.find(path);
        if (it != Writers.end()) {
            return it->second;
        }
        if (!Transaction) {
            return nullptr;
        }
        auto yPath = WorkDir + "/" + path;
        auto richYPath = NYT::TRichYPath(yPath);
        if (Transaction->Exists(yPath)) {
            richYPath.Append(true);
        } else if (!Schema.Empty()) {
            richYPath.Schema(Schema);
            if (OptimizeForScan) {
                richYPath.OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR);
            }
        }
        auto writer = Transaction->CreateTableWriter<NYT::TNode>(richYPath);
        Writers[path] = writer;
        return writer;
    }

private:
    NYT::IClientPtr Client;
    NYT::ITransactionPtr Transaction;
    NYT::TYPath WorkDir;
    NYT::TTableSchema Schema;
    TMap<TString, NYT::TTableWriterPtr<NYT::TNode>> Writers;
    const bool OptimizeForScan;
};
