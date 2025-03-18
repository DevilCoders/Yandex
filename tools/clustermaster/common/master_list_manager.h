#pragma once

#include "list_manager.h"
#include "printer.h"

#include <yweb/config/hostconfig.h>

#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

class THostConfig;
struct TConfigMessage;

class TMasterListManager: public IListManager {
public:
    typedef THashMap<TString, TVector<TString> > THostListForTag;
    typedef THashMap<TString, THostListForTag> THostList;

private:
    typedef THashMap<TString, TSet<TString> > TUsedTagsList;
    typedef THashMap<TString, size_t> TGroupsMap;
    typedef TSet<TString> TGlobalTagsSet;

private:
    struct THostConfigData {
        TString Text;
        THostConfig HostConfig;
    };

    TVector<TSimpleSharedPtr<THostConfigData> > HostConfigs;
    TString HostListText;
    THostList HostList;

    TGroupsMap GroupsMap;

    mutable TUsedTagsList UsedTags;

public:
    TMasterListManager();
    ~TMasterListManager() override;

    void Reset();

    void LoadHostcfgsFromStrings(const TVector<TString>& strings);
    void LoadHostcfgs(const TVector<TString>& filenames);
    void LoadHostcfgFromStream(IInputStream&);
    void LoadHostlist(const TString& filename, const TString& varDirPath);
    void LoadHostlistFromString(const TString&);

    void GetList(const TString& tag, const TVector<TString>& workers, TVector<TString>& output) const;
    void GetList(const TString& tag, const TString& worker, TVector<TString>& output) const {
        TVector<TString> workers;
        workers.push_back(worker);
        GetList(tag, workers, output);
    }
    void GetList(const TString& tag, TVector<TString>& output) const {
        GetList(tag, "", output);
    }

    virtual void GetListForVariable(const TString& tag, TVector<TString>& output) const;

    int GetGroupId(const TString& hostname) const;
    bool CheckGroupDependency(const TString& thishost, const TString& dependhost) const;
    bool ValidateGroupHost(const TString& hostname) const;

    void ExportLists(const TString& worker, TConfigMessage* message) const;

    void DumpState(TPrinter& out) const override;

private:

    void GetHostcfgList(const TString& tag, const TString& worker, TVector<TString>& output, unsigned int hostcfgid) const;
    void GetHostlistList(const TString& tag, const TString& worker, TVector<TString>& output) const;

    void LoadGroups();
};
