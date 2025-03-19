#pragma once

#include <library/cpp/binsaver/bin_saver.h>

typedef  i64 TCatalogCategory;
const TCatalogCategory NULL_CATALOG_CATEGORY = -1;


struct TCatalogCategoryDescr
{
    TCatalogCategory Parent;
    TString Name;
    bool Hide;
    ui32 Size;
    TString UrlName;
    ui32 Type;


    TCatalogCategoryDescr()
        : Parent(NULL_CATALOG_CATEGORY)
        , Name("")
        , Hide(0)
        , Size(0)
        , UrlName("")
        , Type(0)
    {}

    int operator&(IBinSaver& f)
    {
        f.Add(2,&Parent);
        f.Add(3,&Name);
        return 0;
    }
};

typedef THashMap<TCatalogCategory, TCatalogCategoryDescr> TCategoriesMap;

class TCatalogParser : public IObjectBase
{
    OBJECT_METHODS(TCatalogParser);

    TCategoriesMap Categories;

public:
    int operator&(IBinSaver &f) override
    {
        f.Add(2, &Categories);
        return 0;
    }

    TCatalogParser() {}
    TCatalogParser(TString yaCatalogXmlFile); // Path to YaCatalog.xml

    const TCatalogCategoryDescr & GetCategoryDescr(const TCatalogCategory cat) const;

    const TVector<TCatalogCategory> GetCategoryPath(const TCatalogCategory cat) const;

    TCatalogCategory SelectGeneral(const TVector<TCatalogCategory>& categories) const;
    TCatalogCategory SelectGeneral(const TString & categories) const; //comma-separated list

};

