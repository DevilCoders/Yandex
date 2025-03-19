#pragma once

#include <util/generic/string.h>
#include <util/folder/dirut.h>

class TAttrProcessorFlags {
public:
    bool IndexUrl;
    bool SplitUrl;
    bool CutScheeme;
    bool IgnoreDateAttrs;
    bool IndexUrlAttributes;

    TAttrProcessorFlags()
        : IndexUrl(true)
        , SplitUrl(true)
        , CutScheeme(false)
        , IgnoreDateAttrs(false)
        , IndexUrlAttributes(true)
    {
    }
};

struct TAttrProcessorConfig: public TAttrProcessorFlags {
    TString FilterPath;     // $(catalog)/filter.obj
    TString LocalFilterPath;     // $(geohome)/local.filter.obj
    TString CatClosurePath; // $(catalog)/common.c2p
    TString HostAttrsPath;  // $(walrusdir)/host.attrs
    TString MirrorsPath;     // ???/mirrors.trie
    TString AttrsPath;
    TString AttributerDir;
    TString GeoTrieName;
    TString GeoDocsName;
    TString GeoDocsOffName;
    TString HomeDir;
    TString TokenSplitterFile;
    ui32 Cluster;
    bool MapFilter;
    bool CreateMultilanguageDocAttrs;

    TExistenceChecker ExistenceChecker;

    TAttrProcessorConfig()
        : Cluster(0)
        , MapFilter(false)
        , CreateMultilanguageDocAttrs(false)
    {}
};
