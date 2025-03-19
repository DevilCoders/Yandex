#pragma once

#include "builder.h"


class TMultipartArchiveBuilder : public ITextArchiveBuilder {
private:
    class TArcPortionWriter;
public:
    TMultipartArchiveBuilder(const TConstructContext& ctx);

    static ITextArchiveBuilder::TFactory::TRegistrator<TMultipartArchiveBuilder> Registrator;
private:
    void MergePortions(const TMergerContext&) override;
    void RemapDocIds(const TVector<ui32>* remap) override;
private:
    NRTYArchive::TMultipartConfig MultipartConfig;
};
