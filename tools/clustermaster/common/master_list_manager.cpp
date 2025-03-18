#include "master_list_manager.h"

#include "log.h"
#include "make_vector.h"
#include "messages.h"
#include "parsing_helpers.h"
#include "util.h"
#include "vector_to_string.h"

#include <yweb/config/hostconfig.h>

#include <util/folder/path.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/stream/pipe.h>
#include <util/string/printf.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/system/tempfile.h>

namespace {
    void ReadFromFile(const TString& fileName, TString& hostList, TVector<TString>& rewriteCmds, int reclevel, int maxrecursion) {
        if (maxrecursion < 0) {
            ythrow yexception() << "Max recursion level for included files reached";
        }

        TFile file(fileName, OpenExisting | RdOnly);
        TFileInput input(file);

        TString line;

        int lineNumber = 0;

        bool includes = false;

        while (input.ReadLine(line)) {
            ++lineNumber;

            TString lineStripped = StripInPlace(line);

            bool comment = (*lineStripped.begin() == '#');

            if (comment) {
                TStringBuf first, rest;
                SplitByOneOf(lineStripped, " \t", first, rest);

                if (first == "#!REWRITE") {
                    if (!rest) {
                        ythrow yexception() << "REWRITE without parameter in file " << fileName;
                    }

                    rewriteCmds.push_back(TString(rest));
                } else if (first == "#!+INCLUDES") {
                    includes = true;
                } else if (first == "#!-INCLUDES") {
                    includes = false;
                } else {
                    hostList.append(line);
                    hostList.append("\n");
                }
            } else {
                if (includes) {
                    try {
                        ReadFromFile(lineStripped, hostList, rewriteCmds, reclevel+1, maxrecursion-1);
                    } catch (const yexception &e) {
                        ythrow yexception() << "Cannot process file " << lineStripped << " included from " << lineStripped << " in line " << lineNumber << ": " << e.what();
                    }
                } else {
                    hostList.append(line);
                    hostList.append("\n");
                }
            }
        }
    }
}

TMasterListManager::TMasterListManager() {
}

TMasterListManager::~TMasterListManager() {
}

void TMasterListManager::Reset() {
    HostConfigs.clear();
    HostList.clear();
    HostListText = TString();
    GroupsMap.clear();
}

void TMasterListManager::LoadHostcfgsFromStrings(const TVector<TString>& strings) {
    if (!HostConfigs.empty()) {
        ythrow TWithBackTrace<yexception>() << "cannot reload host configs";
    }

    for (TVector<TString>::const_iterator string = strings.begin();
            string != strings.end(); ++string)
    {
        HostConfigs.push_back(new THostConfigData);
        HostConfigs.back()->Text = *string;
        HostConfigs.back()->HostConfig.LoadMem(string->data());
    }

    LoadGroups();

}

void TMasterListManager::LoadHostcfgs(const TVector<TString>& filenames) {
    TVector<TString> strings;
    for (TVector<TString>::const_iterator filename = filenames.begin();
            filename != filenames.end(); ++filename)
    {
        strings.push_back(TUnbufferedFileInput(*filename).ReadAll());
    }
    LoadHostcfgsFromStrings(strings);
}

void TMasterListManager::LoadHostcfgFromStream(IInputStream& input) {
    if (!HostConfigs.empty()) {
        ythrow TWithBackTrace<yexception>() << "cannot reload host configs";
    }

    LoadHostcfgsFromStrings(MakeVector(input.ReadAll()));
}

void TMasterListManager::LoadHostlist(const TString& filename, const TString& varDirPath) {
    TVector<TString> rewriteCmds;
    TString hostList;

    ReadFromFile(filename, hostList, rewriteCmds, 0, 16);

    for (size_t i = 0; i < rewriteCmds.size(); ++i) {
        TTempFile configFile(TString("/tmp/clustermaster-host-list-") + ToString(MicroSeconds()) + ToString(RandomNumber<ui64>()));

        TPipeOutput out(rewriteCmds[i] + " > " + configFile.Name());
        out.Write(hostList.data(), hostList.size());
        out.Close();

        hostList = TUnbufferedFileInput(configFile.Name()).ReadAll();
    }

    if (varDirPath) {
        TFsPath fsPath = TFsPath(varDirPath).Child("host-list.dump");
        fsPath.Parent().MkDirs();
        TOFStream out(fsPath.GetPath());
        out.Write(hostList);
    }

    LoadHostlistFromString(hostList);
}

static void ParseItemList(const TString &input, TVector<TString> &output) {
    TVector<TString> ranges;
    Split(input, ",", ranges);

    for (TVector<TString>::iterator i = ranges.begin(); i != ranges.end(); ++i) {
        size_t splitter;
        if ((splitter = i->find("..")) != TString::npos) {
            // Item range
            TString first = i->substr(0, splitter);
            TString last = i->substr(splitter + 2, TString::npos);

            if (!ParseRange(first, last, output))
                ythrow yexception() << "invalid range in hostlist: " << *i;
        } else {
            output.push_back(*i);
        }
    }
}

void TMasterListManager::LoadHostlistFromString(const TString& string) {
    if (!HostList.empty()) {
        ythrow TWithBackTrace<yexception>() << "cannot load host list more then once";
    }

    HostListText = string;

    TStringInput in(HostListText);

    TString line;
    while (in.ReadLine(line)) {
        size_t commentPosition = line.find('#');
        if (commentPosition != TString::npos) {
            line = line.substr(0, commentPosition);
        }

        if (line.empty())
            continue;

        TVector<TString> words;
        Split(line, " ", words);

        if (words.empty())
            continue;

        if (words.size() == 1) {
            // Single item; Example:
            // host1
            // host2,host3,host4..host9
            TVector<TString> items;
            ParseItemList(words[0], items);

            for (TVector<TString>::const_iterator item = items.begin(); item != items.end(); ++item)
                HostList[""][""].push_back(*item);
        } else if (words[0].find(':') != TString::npos) {
            // Per-host item list; Example:
            // SOMETAG:host1 0 1 2 3
            // SOMETAG:host2 4..7
            // SOMETAG:host3..host4 00..59
            TVector<TString> pair;
            Split(words[0], ":", pair);

            const TString& tag = pair[0];
            if (pair.size() != 2)
                ythrow yexception() << "bad syntax in hostlist file, expected TAG:host specification in a first word: " << line;

            TVector<TString> hosts;
            ParseItemList(pair[1], hosts);

            for (TVector<TString>::iterator host = hosts.begin(); host != hosts.end(); ++host) {
                if (Find(HostList[tag][""].begin(), HostList[tag][""].end(), *host) == HostList[tag][""].end())
                    HostList[tag][""].push_back(*host);

                for (TVector<TString>::const_iterator word = words.begin() + 1; word != words.end(); ++word) {
                    TVector<TString> items;
                    ParseItemList(*word, items);

                    for (TVector<TString>::const_iterator item = items.begin(); item != items.end(); ++item)
                        HostList[tag][*host].push_back(*item);
                }
            }
        } else {
            // Multiple tags with single host (compat); Example:
            // TAG1 TAG2 host1,host2..host4
            // TAG3 TAG4 host5

            // Expand ranges
            TVector<TString> hosts;
            ParseItemList(words.back(), hosts);

            for (TVector<TString>::const_iterator word = words.begin(); word != words.end() - 1; ++word)
                for (TVector<TString>::const_iterator host = hosts.begin(); host != hosts.end(); ++host)
                    HostList[*word][""].push_back(*host);
        }
    }
}

void TMasterListManager::GetList(const TString& tag, const TVector<TString>& workers, TVector<TString>& output) const {
    if (tag.StartsWith("hostcfg")) {
        int id = 0;
        size_t colonpos = tag.find(':');
        if (colonpos == TString::npos)
            ythrow yexception() << "hostcfg used, but no : found to separate tag";

        if (colonpos > 7)
            id = atoi(tag.substr(7, colonpos-7).data());

        if (id > 0)
            --id;

        TString tag0 = tag.substr(colonpos + 1, TString::npos);

        size_t atpos = tag0.find('@');
        Y_VERIFY(workers.size() > 0);
        size_t workerId = atpos != TString::npos ? atoi(tag0.substr(atpos + 1, TString::npos).data()) :
                workers.size() - 1; // last worker if id isn't specified

        if (workerId > workers.size() - 1)
            ythrow yexception() << "Bad worker id (it exceeds number of workers): " << workerId;

        TString tag1 = tag0.substr(0, atpos);

        GetHostcfgList(tag1, workers.at(workerId), output, id);
    } else if (tag.StartsWith("hostlist")) {
        GetHostlistList(tag.size() > 8 && tag[8] == ':' ? tag.substr(9, TString::npos) : "", workers.back(), output);
    } else {
        ythrow yexception() << "unknown table \"" << tag << "\"";
    }
}

void TMasterListManager::GetListForVariable(const TString&, TVector<TString>& /*output*/) const {
}

void TMasterListManager::LoadGroups() {
    if (HostConfigs.empty())
        return;

    // Only first hostcfg is used for groups
    const THostConfig* HostConfig = &HostConfigs[0].Get()->HostConfig;

    for (size_t g = 0; g < HostConfig->GetNumScatterGroups(); ++g) {
        TVector<ui32> ids;

        HostConfig->GetGroupScatterIds(g, ids);
        for (TVector<ui32>::const_iterator i = ids.begin(); i != ids.end(); ++i)
            GroupsMap[HostConfig->GetScatterHost(*i)] = g;

        HostConfig->GetGroupSegmentIds(g, ids);
        for (TVector<ui32>::const_iterator i = ids.begin(); i != ids.end(); ++i)
            GroupsMap[HostConfig->GetSegmentName(*i)] = g;
    }
}

int TMasterListManager::GetGroupId(const TString& hostname) const {
    TGroupsMap::const_iterator g = GroupsMap.find(hostname);

    return g != GroupsMap.end() ? g->second : -1;
}

bool TMasterListManager::CheckGroupDependency(const TString& thishost, const TString& dependhost) const {
    const int thishostGroupId = GetGroupId(thishost);

    return thishostGroupId != -1 && thishostGroupId == GetGroupId(dependhost);
}

bool TMasterListManager::ValidateGroupHost(const TString& hostname) const {
    if (HostConfigs.empty())
        return false;

    // Only first hostcfg is used for groups
    const THostConfig* HostConfig = &HostConfigs[0].Get()->HostConfig;

    for (size_t i = 0; i < HostConfig->GetNumSegments(); ++i)
        if (hostname == HostConfig->GetSegmentName(i))
            return true;

    for (size_t i = 0; i < HostConfig->GetNumScatterHosts(); ++i)
        if (hostname == HostConfig->GetScatterHost(i))
            return true;

    return false;
}

void TMasterListManager::ExportLists(const TString& worker, TConfigMessage* message) const {
    Y_UNUSED(worker);

    for (TVector<TSimpleSharedPtr<THostConfigData> >::const_iterator hostConfig = HostConfigs.begin();
            hostConfig != HostConfigs.end(); ++hostConfig)
    {
        message->AddHostCfgs((*hostConfig)->Text);
    }

    message->SetHostList(HostListText);
}

inline const TString& CheckWorker(const TString& worker) {
    if (worker.empty())
        ythrow yexception() << "No worker hostname for per-host list; are you using per-host list as hosts list?";
    return worker;
}

void TMasterListManager::GetHostcfgList(const TString& tag, const TString& worker, TVector<TString>& output, unsigned int hostcfgid) const {
    if (hostcfgid >= HostConfigs.size())
        ythrow yexception() << "table from hostconfig (#" << hostcfgid + 1 << ") requested, but hostconfig not loaded";

    const THostConfig* HostConfig = &HostConfigs.at(hostcfgid).Get()->HostConfig;

    if (tag == "h") {
        for (size_t i = 0; i < HostConfig->GetNumSegments(); ++i)
            output.push_back(HostConfig->GetSegmentName(i));
    } else if (tag == "W") {
        for (size_t i = 0; i < HostConfig->GetNumScatterHosts(); ++i)
            output.push_back(HostConfig->GetScatterHost(i));
    } else if (tag[0] == 'F') {
        const TVector<const char*>& list = HostConfig->GetFullList(tag.substr(1, TString::npos).data());

        for (TVector<const char*>::const_iterator i = list.begin(); i != list.end(); ++i)
            output.push_back(*i);
    } else if (tag[0] == 'C') {
        const TString type(tag.substr(1, TString::npos));
        if (!type.empty()) {
            const TClusterRanges& ranges = HostConfig->GetFriendsHostsClusterRanges(CheckWorker(worker).data(), type.data());
            for (const auto& range: ranges) {
                for (int i = range.Start; i <= range.End; ++i) {
                    output.push_back(Sprintf("%03d", i));
                }
            }
            return;
        }
        const ClusterConfig* cc = HostConfig->GetSegmentClusterConfig(CheckWorker(worker).data());
        if (!cc)
            ythrow yexception() << "cannot get cluster config";

        for (int i = 0; i < cc->Clusters(); i++)
            output.push_back(Sprintf("%03d", i));
    } else if (tag == "S") {
        const SearchClusterConfig *scc = HostConfig->GetSegmentSearchClusterConfig(CheckWorker(worker).data());
        if (!scc)
            ythrow yexception() << "cannot get search cluster config";

        for (int i = 0; i < scc->Clusters(); i++)
            output.push_back(Sprintf("%03d", i));
    } else if (tag == "u") {
        const SearchClusterConfig* scc = HostConfig->GetSegmentSearchClusterConfig(CheckWorker(worker).data());
        if (!scc)
            ythrow yexception() << "cannot get search cluster config";

        for (int i = 0; i < scc->UploadClusters(); i++)
            output.push_back(Sprintf("%03d", i));
    } else if (tag == "M") {
        TVector<ui32> ids;
        HostConfig->GetScatterIds(HostConfig->GetSegmentId(CheckWorker(worker).data()), ids);

        for (TVector<ui32>::iterator i = ids.begin(); i != ids.end(); i++)
            output.push_back(HostConfig->GetScatterHost(*i));
    } else if (tag == "Mn") {
        TVector<ui32> ids;
        HostConfig->GetScatterIds(HostConfig->GetSegmentId(CheckWorker(worker).data()), ids);

        for (unsigned int i = 0; i < ids.size(); i++)
            output.push_back(Sprintf("%03d", i));
    } else if (tag == "j") {
        TVector<ui32> notSpamScatterPieces;
        HostConfig->GetNotSpamScatterPieces(CheckWorker(worker).data(), notSpamScatterPieces);

        for (TVector<ui32>::iterator i = notSpamScatterPieces.begin(); i != notSpamScatterPieces.end(); i++)
            output.push_back(Sprintf("%03d", *i));
    } else if (tag == "i" || tag == "X") {
        for (unsigned int i = 0, e = HostConfig->GetNumScatterPieces(CheckWorker(worker).data()); i < e; i++)
            output.push_back(Sprintf("%03d", i));
    } else if (tag == "spider") {
        output.push_back(HostConfig->GetFriends(CheckWorker(worker).data()).GetFriends("spider"));
    } else if (tag == "zm") {
        output.push_back(HostConfig->GetFriends(CheckWorker(worker).data()).GetFriends("zm"));
    } else if (tag == "htarc") {
        output.push_back(HostConfig->GetFriends(CheckWorker(worker).data()).GetFriends("htarc"));
    } else if (tag == "orca") {
        output.push_back(HostConfig->GetFriends(CheckWorker(worker).data()).GetFriends("orca"));
    } else if (tag == "b" || tag == "B") {
        TVector<ui32> ids;
        if (tag == "B")
            HostConfig->GetFriendSegs(CheckWorker(worker).data(), ids);
        else
            HostConfig->GetSegmentIds(HostConfig->GetScatterId(CheckWorker(worker).data()), ids);
        for (size_t i = 0; i < ids.size(); i++)
            output.push_back(HostConfig->GetSegmentName(ids[i]));
    } else if (tag == "J") {
        TVector<ui32> list;
        HostConfig->GetNotSpamScatterPiecesOnScatter(CheckWorker(worker).data(), list);
        for (unsigned int i = 0; i < list.size(); ++i)
            output.push_back(Sprintf("%d", list[i]));
    } else if (tag == "K") {
        TVector<ui32> list;
        HostConfig->GetSpamScatterPiecesOnScatter(CheckWorker(worker).data(), list);
        for (unsigned int i = 0; i < list.size(); ++i)
            output.push_back(Sprintf("%d", list[i]));
    } else {
        const char* sh = HostConfig->GetSpecialHost(tag.data());
        if (sh)
            output.push_back(sh);
        else
            ythrow yexception() << "unknown hostcfg tag \"" << tag << "\"";
    }
}

void TMasterListManager::GetHostlistList(const TString& tag, const TString& worker, TVector<TString>& output) const {
    TString stripped_tag = tag;

    bool clusters = false;

    size_t pos = tag.find(":clusters");
    if (pos != TString::npos) {
        stripped_tag = tag.substr(0, pos);
        clusters = true;
    }

    THostList::const_iterator fortag = HostList.find(stripped_tag);
    if (fortag == HostList.end())
        ythrow yexception() << (HostList.empty() ? TString("hostlist is empty") : "unknown hostlist tag \"" + stripped_tag + "\"");

    THostListForTag::const_iterator list;

    if (clusters) {
        if (worker.empty())
            ythrow yexception() << "no worker hostname for hostlist clustets; shouldn't happen";

        list = fortag->second.find(worker);

        if (list == fortag->second.end())
            ythrow yexception() << "no clusters defined for worker in hostlist for tag \"" << stripped_tag << "\" for worker \"" << worker << "\"";
    } else {
        list = fortag->second.find("");

        if (list == fortag->second.end())
            ythrow yexception() << "no data found in hostlist tag \"" << tag << "\"";
    }

    for (TVector<TString>::const_iterator i = list->second.begin(); i != list->second.end(); ++i)
        output.push_back(*i);
}

void TMasterListManager::DumpState(TPrinter& out) const {
    out.Println("ListManager:");
    TPrinter l1 = out.Next();
    l1.Println("Groups:");
    TPrinter l2 = l1.Next();
    for (TGroupsMap::const_iterator group = GroupsMap.begin(); group != GroupsMap.end(); ++group) {
        l2.Println(group->first + " " + ToString(group->second));
    }
    out.Println("ListManager.");
}
