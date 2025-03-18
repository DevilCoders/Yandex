#include <sys/types.h>
#include <dirent.h>

#include <util/digest/murmur.h>
#include <util/folder/dirut.h>
#include <util/stream/file.h>
#include <util/generic/hash_set.h>
#include <util/string/split.h>
#include <util/system/sysstat.h>
#include <util/system/fs.h>

#include "comm.h"
#include "client.h"
#include "distribute.h"
#include "parallel.h"

TString TempRoot;

TString GetPathForFile(const TString& fn, const TString& user)
{
    if (fn[0] == '/')
        return fn;
    TString dirName = TempRoot + "/" + user;
    MakePathIfNotExist(dirName.data());
    return dirName + "/" + fn;
}

void MakeStoragePath()
{
    MakePathIfNotExist(TempRoot.data());
}

bool StoreFile(const TString& fn, const TString& hash, TVector<char>::const_iterator start, TVector<char>::const_iterator end)
{
    TString tempPath = fn + "." + ::ToString(hash) + ".new";
    if (NFs::Exists(tempPath)) {
        if (IsDir(tempPath))
            RemoveDirWithContents(tempPath);
        else
            remove(tempPath.data());
    }
    TString path = fn + "." + ::ToString(hash);
    if (NFs::Exists(path)) {
        if (IsDir(path))
            RemoveDirWithContents(path);
        else
            remove(path.data());
    }

    try {
        TOFStream dataFile(tempPath);
        dataFile.Write(start, end - start);
        Chmod(tempPath.data(), MODE0777);
        rename(tempPath.data(), (fn + "." + ::ToString(hash)).data());
        // symlink
        remove(fn.data());
        size_t pos = fn.find_last_of('/');
        NFs::SymLink((pos != TString::npos ? fn.substr(pos + 1) : fn) + "." + ::ToString(hash), fn);
    }
    catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return false;
    }
    return true;
}

void SetStoragePath(TString path)
{
    TempRoot = path;
    MakeStoragePath();
}

void CleanUp()
{
    if (NFs::Exists(TempRoot)) {
        if (IsDir(TempRoot))
            RemoveDirWithContents(TempRoot);
        else
            remove(TempRoot.data());
    }
}

//TString Ip4Host(TString name)
//{
//    hostent* host = gethostbyname(~name);
//
//    if (!host)
//        return name;
//    in_addr* address = (in_addr *)host->h_addr;
//    return inet_ntoa(*address);
//}

const size_t BASE = 5;

double HostDistance (THostStatus* s1, THostStatus* s2)
{
    // really hacky and might not really work
    i64 diff = s1->Address.Interface - s2->Address.Interface;
    return diff > 0 ? diff : -diff;
}

THostStatus* Median(TVector<THostStatus*>& chunk)
{
    if (chunk.size() == 1)
        return chunk[0];
    TVector<double> distsums;
    distsums.resize(chunk.size(), 0.);
    for (size_t i = 0; i < chunk.size(); i++) {
        for (size_t j = i + 1; j < chunk.size(); j++) {
            const double distance = HostDistance(chunk[i], chunk[j]);
            distsums[i] += distance;
            distsums[j] += distance;
        }
    }
    TVector<double>::iterator median = std::min_element(distsums.begin(), distsums.end());
    return chunk[median - distsums.begin()];
}

void Cluster(int clustCount, TVector<THostStatus>& items, TVector<TVector<THostStatus*> >* chunks)
{
    chunks->resize(clustCount);
    { // initial random distribution
        TVector<THostStatus>::iterator iter = items.begin();
        int index = 0;
        while (iter != items.end())
            chunks->at(index++ % clustCount).push_back(iter++);
    }

    int step = 10;
    while (step--) {
        { // medians
            TVector<TVector<THostStatus*> >::iterator iter = chunks->begin();
            while (iter != chunks->end()) {
//                THostStatus ** x = iter->begin();
//
//                while(x != iter->end()) {
//                    Cout << "\t" << (*x)->Host << "(" << Ip4Host((*x)->Host)<< "/" << ntohl((*x)->Address.IP) << ")";
//                    x++;
//                }
//                Cout << Endl;
                THostStatus* median = Median(*iter);
                iter->resize(0);
                iter->push_back(median);
                iter++;
            }
        }

        { // new distribution
            TVector<THostStatus>::iterator iter = items.begin();
            while (iter != items.end()) {
                TVector<TVector<THostStatus*> >::iterator chit = chunks->begin();
                TVector<THostStatus*>* closeMost = nullptr;
                double distance = 0;

                while (chit != chunks->end()) {
                    if (chit->at(0) == iter) { // median
                        closeMost = nullptr;
                        break;
                    }

                    const double d = HostDistance(iter, chit->at(0));
                    if (!closeMost || distance > d) {
                        closeMost = chit;
                        distance = d;
                    }
                    chit++;
                }
                if (closeMost)
                    closeMost->push_back(iter);
                iter++;
            }
        }
    }
    TVector<TVector<THostStatus*> >::iterator iter = chunks->begin();
    while (iter != chunks->end()) {
        if (iter->empty()) {
            iter = chunks->erase(iter);
        }
        else iter++;
    }
}

struct TDistrTask {
    THostStatus* Host;
    TVector<THostStatus*>* RedistributeTo;

    TLazyContentsGetter* Contents;
    TString Hash;
    TString File;
    TString ID;

    TVector<TString> Failures;
};

struct TDistr
{
    bool operator ()(TDistrTask* task)
    {
        if (CommunicateRC(task->Host->Address, "check\t" + GenID("check") + "\t" + task->File + "\t" + task->Hash) != 0) {
            TPack pack = new TVector<char>;
            const char *receive = "receive";
            pack->insert(pack->end(), receive, receive + strlen(receive));
            pack->insert(pack->end(), '\t');
            pack->insert(pack->end(), task->ID.data(), task->ID.data() + task->ID.size());
            pack->insert(pack->end(), '\t');
            pack->insert(pack->end(), task->File.data(), task->File.data() + task->File.size());
            pack->insert(pack->end(), '\t');
            pack->insert(pack->end(), task->Hash.data(), task->Hash.data() + task->Hash.size());
            pack->insert(pack->end(), '\t');
            const TVector<char>* contents = task->Contents->GetContents();
            pack->insert(pack->end(), contents->begin(), contents->end());
            TString msg = Communicate(task->Host->Address, pack);

            TVector<TString> parts;
            StringSplitter(msg).Split('\t').SkipEmpty().Collect(&parts);
            TString rc = !msg.empty() ? (parts.size() > 1 ? parts[1] : parts[0]) : "1";
            if (atoi(rc.data()) != 0) {
                TVector<THostStatus*>::iterator iter = task->RedistributeTo->begin();
                while (iter != task->RedistributeTo->end()) {
                    task->Failures.push_back((*iter)->Host);
                    iter++;
                }
                task->Failures.push_back(task->Host->Host);
                return false;
            }
        }
        if (task->RedistributeTo->empty())
            return true;
        TString msg = "redistribute\t" + task->ID + "\t" + task->File + "\t" + task->Hash;
        TVector<THostStatus*>::iterator iter = task->RedistributeTo->begin();
        while (iter != task->RedistributeTo->end()) {
            msg += "\t";
            msg += (*iter)->Host;
//            msg += "(" + Ip4Host((*iter)->Host) + ")";
            iter++;
        }
        if (Verbose)
            Clog << task->Host->Host << "<-" << msg << Endl;

        TString status = Communicate(task->Host->Address, msg);
        TVector<TString> vs;
        StringSplitter(status).Split('\t').SkipEmpty().Collect(&vs);
        if (!vs.empty() && !atoi(vs[1].data()))
            return true;
        task->Failures.insert(task->Failures.end(), &vs[2], vs.end());
        return false;
    }
};

void Distribute(const TString& src, const TVector<TString>& hosts, TVector<TString>* fails, TString* hash, const TString& id)
{
    TString file = src.find_last_of('/') != TString::npos ? src.substr(src.find_last_of('/') + 1) : src;
    TLazyContentsGetter contents(src);
    *hash = GetHashForFileContents(file, contents.GetContents()->begin(), contents.GetContents()->end());
    Distribute(file, *hash, contents, hosts, fails, id);
}

void Distribute(const TString& file, const TString& hash, TLazyContentsGetter& content, const TVector<TString>& hosts, TVector<TString>* fails, const TString& id)
{
    fails->resize(0);
//    fails->insert(fails->begin(), hosts.begin(), hosts.end());

    TVector<THostStatus> statuses;
    {
        TVector<TString>::const_iterator iter = hosts.begin();
        while (iter != hosts.end()) {
            statuses.push_back(THostStatus(*iter++));
        }
    }

    TVector<TVector<THostStatus*> > chunks;
    size_t clustCount = std::min(BASE, hosts.size());
    Cluster(clustCount, statuses, &chunks);
    TVector<TDistrTask*> tasks;
    {
        TVector<TVector<THostStatus*> >::iterator iter = chunks.begin();
        while (iter != chunks.end()) {
            TDistrTask* task = new TDistrTask;
            task->Host = iter->at(0);
            iter->erase(iter->begin());
            task->RedistributeTo = iter;
            task->Contents = &content;
            task->Hash = hash;
            task->File = file;
            task->ID = id;

            tasks.push_back(task);
            iter++;
        }
    }

    ParallelTask<TDistrTask*, TDistr>(tasks, &tasks, clustCount);

    TVector<TDistrTask*>::iterator iter = tasks.begin();
    while (iter != tasks.end()) {
        fails->insert(fails->end(), (*iter)->Failures.begin(), (*iter)->Failures.end());
        delete *iter;
        iter++;
    }
}
