#pragma once

#include <kernel/snippets/iface/geobase.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

class TImagesGeobase : public NSnippets::IGeobase {
public:
    TImagesGeobase(const TString& geoaFile);
    ~TImagesGeobase() override;

public:
    bool Contains(TCateg biggerCateg, TCateg smallerCateg) const override;
    TCateg Categ2Parent(TCateg categ) const override;

private:
    class TImpl;
    THolder<TImpl> Impl;
};
