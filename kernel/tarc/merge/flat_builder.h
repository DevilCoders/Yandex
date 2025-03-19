#pragma once

#include <kernel/tarc/merge/builder.h>

class TFlatArchiveBuilder : public ITextArchiveBuilder {
private:
    class TArcPortionWriter;

private:
    const ui32 ArchiveVersion;

public:
    TFlatArchiveBuilder(const TConstructContext& ctx);

    static ITextArchiveBuilder::TFactory::TRegistrator<TFlatArchiveBuilder> Registrator;
private:
    void MergePortions(const TMergerContext&) override;
    void RemapDocIds(const TVector<ui32>* remap) override;
};
