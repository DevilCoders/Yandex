#include "qd_source_updater.h"
#include "qd_reporters.h"

#include <kernel/searchlog/errorlog.h>

#include <util/draft/datetime.h>
#include <util/folder/path.h>
#include <util/string/builder.h>
#include <util/string/strip.h>
#include <util/system/mlock.h>

namespace NQueryData {

    static TString TrieTooBig(const TString& trie, const TServerOpts& opts, ui64 predictedRAMSize) {
        return TStringBuilder()
                << "trie '" << trie << "' is too big to load in memory (TrieRAMSize: "
                << predictedRAMSize << ", " << opts.Report() << ")";
    }

    static bool AllowTrieRead(const TServerOpts& opts, ui64 predictedRAMSize, ELoadMode mode) {
        return mode == LM_MMAP || predictedRAMSize <= opts.MaxTrieRAMSize;
    }

    NChecker::EUpdateStrategy TSourceChecker::GetUpdateStrategy(const TFile&, const TFileStat& fs, const TSharedObjectPtr&) const {
        ui64 curmem = CurrentMemoryUsage();

        if (fs.Size + curmem <= Opts.MaxTotalRAMSize) {
            return NChecker::US_READ_AND_SWAP;
        }

        return NChecker::US_DROP_AND_READ;
    }

    TSourceChecker::TSharedObjectPtr TSourceChecker::MakeNewObject(
                    const TFile& file, const TFileStat& fs, TSourceChecker::TSharedObjectPtr, bool) const
    {
        const TString& fileName = GetMonitorFileName();
        SEARCH_INFO << "loading trie: '" << fileName << "'";

        try {
            Y_ENSURE(fs.Size, "bad file handle");

            TBlob data = TBlob::PrechargedFromFile(file);
            TSourceChecker::TSharedObjectPtr src = new TSource();
            src->Parse(data, fileName, fs.MTime);

            ELoadMode loadMode = LM_RAM;

            if (Opts.AlwaysUseMMap) {
                loadMode = LM_MMAP;
            } else if (Opts.EnableFastMMap && fs.Size >= Opts.MinTrieFastMMapSize && src->SupportsFastMMap()) {
                loadMode = LM_FAST_MMAP;
            }

            ui64 predictedRAMSize = src->PredictRAMSize(loadMode);

            Y_ENSURE(AllowTrieRead(Opts, predictedRAMSize, loadMode),
                     TrieTooBig(fileName, Opts, predictedRAMSize));

            if (LM_RAM == loadMode) {
                if (Opts.LockInRAM) {
                    try {
                        src->SetLockedBlob(data);
                    } catch (const yexception& e) {
                        SEARCH_ERROR << "failed to lock in memory '" << fileName << "'";
                        src->SetUnlockedBlob(TBlob::FromFileContent(file));
                    }
                } else {
                    src->SetUnlockedBlob(TBlob::FromFileContent(file));
                }
            }

            src->InitTrie(loadMode);

            NSc::TValue rep;
            ReportRealFile(rep, fileName);
            rep.MergeUpdate(src->GetStats(SV_MAIN));

            SEARCH_INFO << "done loading trie: '" << fileName << "' (" << rep.ToJson() << ")";

            return src;
        } catch (...) {
            SEARCH_ERROR << "got error while reading '" << fileName << "': " << CurrentExceptionMessage();

            if (Opts.FailOnDataLoadErrors) {
                Y_FAIL("forced fail on data load error");
            }
        }

        return nullptr;
    }

    void TSourceList::GetNewFileSet(const TFile& fh, TSourceList::TFileSet& fs) const {
        TString file = GetMonitorFileName();
        SEARCH_INFO << "updating trie list: '" << file << "'";

        try {
            TFileInput fi(TFile(fh.Duplicate()));
            TString line;
            TFsPath dir = TFsPath(file).RealPath().Dirname(); // directory where the trie list is found
            TStringBuilder sb;

            while (fi.ReadLine(line)) {
                StripInPlace(line);

                if (!line) {
                    continue;
                }

                line.resize(Min(line.find('\t'), line.size()));
                TString triePath = line.StartsWith('/') ? line : (dir / line).GetPath();
                fs.insert(std::make_pair(triePath, SourceOpts));

                if (sb) {
                    sb << ",";
                }
                sb << triePath;
            }

            SEARCH_INFO << "done updating trie list: '" << file << "': (" << (TString&)(sb) << ")";
        } catch(...) {
            SEARCH_ERROR << "got error while reading '" << file << "': " << CurrentExceptionMessage();

            if (SourceOpts.FailOnDataLoadErrors) {
                Y_FAIL("forced fail on data load error");
            }
        }
    }

}
