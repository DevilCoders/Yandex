#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>

#include <kernel/snippets/factors/factors.h>

#include <util/generic/string.h>

#include "factor_domains.h"


const TFactorDomain& GetDomain(EFactorDomain domain) {
    switch (domain) {
        case EFactorDomain::Algo2Domain: return NSnippets::algo2Domain;
        case EFactorDomain::Algo2PlusWebDomain: return NSnippets::algo2PlusWebDomain;
        case EFactorDomain::Algo2PlusWebNoClickDomain: return NSnippets::algo2PlusWebNoClickDomain;
        case EFactorDomain::Algo2PlusWebV1Domain: return NSnippets::algo2PlusWebV1Domain;
    }
}


TVector<float> RewriteFactors(const TFactorDomain& domain, const THashMap<TString, float>& namedFactors) {
    TVector<float> factors(domain.Size(), 0);
    for (TFactorDomain::TIterator it = domain.Begin(); it.Valid(); it.Next()) {
         const auto& fInfo = it.GetFactorInfo();
         if (auto namedIterator = namedFactors.find(fInfo.GetFactorName()); namedIterator != namedFactors.end()) {
            factors[it.GetIndex()] = namedIterator->second;
         }
    }
    return factors;
}


struct TAppConfig {
    TString Cluster;
    TString SourceTable;
    TString DestinationTable;
    TString SourceColumn;
    TString DestinationColumn;
    TString SliceColumn;
    EFactorDomain Domain;

    static TAppConfig FromCommandLint(int argc, const char ** argv) {
        TAppConfig config;
        auto opts = NLastGetopt::TOpts::Default();
        opts.AddLongOption('c', "cluster", "YT cluster")
            .DefaultValue("hahn")
            .StoreResult(&config.Cluster);

        opts.AddLongOption('s', "source-table")
            .Required()
            .StoreResult(&config.SourceTable);

        opts.AddLongOption('d', "destination-table")
            .Required()
            .StoreResult(&config.DestinationTable);

        opts.AddLongOption("source-column")
            .Required()
            .StoreResult(&config.SourceColumn);

        opts.AddLongOption("destination-column")
            .Required()
            .StoreResult(&config.DestinationColumn);

        opts.AddLongOption("slice-column")
            .DefaultValue("slices")
            .StoreResult(&config.SliceColumn);

        opts.AddLongOption("domain")
            .DefaultValue(EFactorDomain::Algo2PlusWebNoClickDomain)
            .StoreResult(&config.Domain);

        NLastGetopt::TOptsParseResult{&opts, argc, argv};

        return config;
    }
};

class TFactorsRewriter final : public NYT::IMapper<NYT::TNodeReader, NYT::TNodeWriter> {
public:
    TFactorsRewriter() = default;

    TFactorsRewriter(
        const TString& sourceColumn,
        const TString& destinationColumn,
        const TString& sliceColumn,
        EFactorDomain domain
    )
        : SourceColumn_(sourceColumn)
        , DestinationColumn_(destinationColumn)
        , SliceColumn_(sliceColumn)
        , Domain_(domain)
    {
    }

    Y_SAVELOAD_JOB(SourceColumn_, DestinationColumn_, SliceColumn_, Domain_);

    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            writer->AddRow(DoOneRow(reader->MoveRow()));
        }
    }

    NYT::TNode DoOneRow(NYT::TNode row) const {
        const TFactorDomain& domain = GetDomain(Domain_);
        auto namedFactors = GetNamedFactors(row[SourceColumn_].AsString());
        TVector<float> factors = RewriteFactors(domain, namedFactors);
        auto factorsNode = NYT::TNode::CreateList();
        for (auto factor : factors) {
            factorsNode.Add(factor);
        }

        row[DestinationColumn_] = std::move(factorsNode);
        row[SliceColumn_] = SerializeFactorBorders(domain.GetBorders(), NFactorSlices::ESerializationMode::LeafOnly);
        return row;
    }

    void PrepareOperation(const NYT::IOperationPreparationContext& context, NYT::TJobOperationPreparer& preparer) const override {
        if (context.GetInputCount() == 0) {
            preparer.NoOutputSchema(0);
            return;
        }

        auto inputSchema = context.GetInputSchema(0);
        if (inputSchema.Empty()) {
            preparer.NoOutputSchema(0);
            return;
        }


        NYT::TTableSchema outputSchema;
        for (auto& column : inputSchema.MutableColumns()) {
            column.ResetSortOrder();
            if (column.Name() != DestinationColumn_) {
                outputSchema.AddColumn(column);
            }
        }

        outputSchema.AddColumn(SliceColumn_, NTi::String());
        outputSchema.AddColumn(DestinationColumn_, NTi::List(NTi::Float()));
        preparer.OutputSchema(0, std::move(outputSchema));
    }


private:
    static THashMap<TString, float> GetNamedFactors(const TStringBuf factorsWithNames) {
        THashMap<TString, float> namedFactors;
        for (TStringBuf factorWithValue : StringSplitter(factorsWithNames).Split('\t')) {
            if (factorWithValue.empty()) {
                continue;
            }
            TStringBuf factorName;
            float factorValue;
            StringSplitter(factorWithValue).Split(':').CollectInto(&factorName, &factorValue);
            if (auto p = namedFactors.emplace(factorName, factorValue); !p.second) {
                ythrow yexception() << "duplicated name: " << factorName;
            }
        }
        return namedFactors;
    }

    TString SourceColumn_;
    TString DestinationColumn_;
    TString SliceColumn_;
    EFactorDomain Domain_;
};


REGISTER_MAPPER(TFactorsRewriter);


int main(int argc, const char ** argv) {
    NYT::Initialize(argc, argv, NYT::TInitializeOptions().CleanupOnTermination(true));
    auto config = TAppConfig::FromCommandLint(argc, argv);

    NYT::IClientPtr ytClient = NYT::CreateClient(config.Cluster);
    ytClient->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(config.SourceTable)
            .AddOutput<NYT::TNode>(config.DestinationTable),
            new TFactorsRewriter(config.SourceColumn, config.DestinationColumn, config.SliceColumn, config.Domain)
    );

    return 0;
}
