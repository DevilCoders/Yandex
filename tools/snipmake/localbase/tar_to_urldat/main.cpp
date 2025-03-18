#include <util/stream/file.h>
#include <util/stream/buffered.h>
#include <util/memory/blob.h>
#include <util/generic/yexception.h>
#include <kernel/tarc/iface/farcface.h>
#include <util/generic/buffer.h>
#include <library/cpp/microbdb/safeopen.h>
#include <yweb/robot/dbscheeme/baserecords.h>
#include <yweb/robot/dbscheeme/mergecfg.h>
#include <util/string/printf.h>
#include <library/cpp/string_utils/url/url.h>

struct TMapping
{
    TVector<TString> Entries;
    TVector<int> D2C;
    THashMap<TString, int> Map;
    TVector<int> IndexToId;

    int Add(const TStringBuf& nameBuf, int docId)
    {
        TString name(nameBuf);
        int result;
        IndexToId.push_back(docId);
        THashMap<TString, int>::iterator ii = Map.find(name);
        if (ii == Map.end()) {
            result = Entries.ysize();
            Map[name] = result;
            Entries.push_back(name);
        }
        else {
            result = ii->second;
        }
        D2C.push_back(result);
        return result;
    }

    void WriteC2N(const TString& filename)
    {
        TFixedBufferFileOutput out(filename);
        for (int i = 0; i < Entries.ysize(); ++i) {
            out << i << "\t" << Entries[i] << Endl;
        }
    }

    void WriteD2C(const TString& filename)
    {
        TFixedBufferFileOutput out(filename);
        for (int i = 0; i < D2C.ysize(); ++i) {
            out << IndexToId[i] << "\t" << D2C[i] << Endl;
        }
    }
};

int MainProtected(int argc, char* argv[])
{
    if (argc != 4) {
        Cerr << Sprintf("Usage: %s <input_tagarchive> <output-cluster-dir> <output-categories-dir>", argv[0]) << Endl;
        Cerr << "url.dat and multilanguagedoc.dat will be written to output-cluster-dir" << Endl;
        Cerr << "{h,d}.{c2n,d2c} will be written to output-categories-dir" << Endl;
        return 0;
    }

    TString inFileName = TString(argv[1]);
    TString clusterdir = TString(argv[2]) + "/";
    TString attrdir = TString(argv[3]) + "/";

    TString outUrlFileName = clusterdir + "url.dat";
    TString outMultilangGroupsName = clusterdir + "multilanguagedoc.dat";

    TOutDatFile<TUrlRec> outUrlFile(outUrlFileName, dbcfg::pg_url, dbcfg::fbufsize, 0);
    TOutDatFile<TDocGroupRec> outMultilangGroups(outMultilangGroupsName, dbcfg::pg_docgrp, dbcfg::fbufsize, 0);

    TFullArchiveIterator ii;
    outUrlFile.Open(outUrlFileName);
    outMultilangGroups.Open(outMultilangGroupsName);
    ii.Open(inFileName.data());

    TMapping dMap;
    TMapping hMap;

    while (ii.Next()) {
        TFullArchiveDocHeader *header = ii.GetFullHeader();
        TString url(header->Url);
        TStringBuf host = GetOnlyHost(url);

        TUrlRec rec;
        memset(&rec, 0, sizeof(TUrlRec));
        rec.ModTime = header->IndexDate;
        rec.DocId = ii.GetDocId();
        rec.Hops = 1;
        strcpy(rec.Name, header->Url);
        rec.UrlId = rec.DocId;
        rec.AddTime = rec.ModTime;
        rec.HttpModTime = rec.ModTime;
        rec.LastAccess = rec.ModTime;
        rec.HostId = hMap.Add(host, rec.DocId);
        rec.HttpCode = 200;
        rec.MimeType = header->MimeType;
        rec.Encoding = header->Encoding;
        rec.Language = header->Language; // what about Language2?
        rec.Flags = 0;
        rec.Status = UrlStatus::INDEXED;
        rec.Size = ii.GetDocLen();

        dMap.Add(GetDomain(host), rec.DocId);
        TUrlExtData extData;
        extData.SetLastAccessTrue(rec.LastAccess);
        outUrlFile.Push(&rec, &extData);
        TDocGroupRec group;
        group.DocId = rec.DocId;
        group.GroupId = rec.DocId;
        outMultilangGroups.Push(&group);
    }

    outUrlFile.Close();
    outMultilangGroups.Close();
    dMap.WriteC2N(attrdir + "d.c2n");
    dMap.WriteD2C(attrdir + "d.d2c");
    hMap.WriteC2N(attrdir + "h.c2n");
    hMap.WriteD2C(attrdir + "h.d2c");

    return 0;
}

int main(int argc, char* argv[])
{
    try {
        return MainProtected(argc, argv);
    }
    catch (const std::exception& e) {
        fprintf(stderr, "Exception: %s\n", e.what());
        return 1;
    }
    catch (...) {
        fprintf(stderr, "Unknown exception\n");
        return 1;
    }
}
