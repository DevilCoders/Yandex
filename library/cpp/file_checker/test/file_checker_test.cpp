#include <library/cpp/file_checker/registry_checker.h>

#include <util/folder/dirut.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/random/random.h>

namespace NChecker {
    struct TFileMutator : TAtomicRefCount<TFileMutator>, NChecker::IUpdater {
        TFileMutator(TString fname, ui32 size)
            : File(fname)
            , FileNew(fname + ".new")
            , Size(size)
            , Checker(*this, 1)
        {
        }

        ~TFileMutator() override {
            Checker.Stop();
        }

        void Start() {
            Checker.Start();
        }

        bool IsChanged() const override {
            return true;
        }

        bool Update() override {
            ui32 any = 1 + RandomNumber(Size);

            unlink(FileNew.data());

            {
                TFixedBufferFileOutput fout(FileNew);

                for (ui32 i = any; i; --i) {
                    fout << i << "\n";
                }
            }

            rename(FileNew.data(), File.data());

            return true;
        }

        TString File;
        TString FileNew;

        ui32 Size;

        TPeriodicChecker Checker;
    };

    struct TRegistryMutator {
        TRegistryMutator(TString fname, ui32 fsize, const TVector<TString>& files)
            : RegistryFile(fname)
            , RegistryFileNew(fname + ".new")
            , Files(files)
        {
            unlink(RegistryFile.data());
            unlink(RegistryFileNew.data());
            for (ui32 i = 0, sz = Files.size(); i < sz; ++i) {
                unlink(Files[i].data());
                unlink((Files[i] + ".new").data());
                FileMutators.push_back(new TFileMutator(Files[i], fsize));
                FileMutators.back()->Start();
            }
        }

        void Update() {
            ui32 any = 1 + RandomNumber(Files.size());

            unlink(RegistryFileNew.data());

            {
                TFixedBufferFileOutput fout(RegistryFileNew);

                fout << any << "\n";

                for (ui32 i = 0; i < any; ++i) {
                    fout << Files[i] << "\n";
                }
            }

            rename(RegistryFileNew.data(), RegistryFile.data());

            Clog.Write(Sprintf("Updated registry %s %u\n", RegistryFile.data(), any));
        }

        TString RegistryFile;
        TString RegistryFileNew;
        TVector<TString> Files;

        TVector<TIntrusivePtr<TFileMutator>> FileMutators;
    };

    struct TCheckerOpts : TFileCheckerOpts {
        bool DropOld;

        TCheckerOpts()
            : TFileCheckerOpts(TFileOptions::GetZeroOnDelete(), 1)
            , DropOld()
        {
        }
    };

    struct TStringHolder : TAtomicRefCount<TStringHolder> {
        TString String;

        TStringHolder(const TString& s)
            : String(s)
        {
        }
    };

    struct TFileReader : TFileChecker<TStringHolder> {
        bool DropOld;

        TFileReader(const TString& fname, const TCheckerOpts& o)
            : TFileChecker<TStringHolder>(fname, o.Options, o.Timer)
            , DropOld(o.DropOld)
        {
        }

        ~TFileReader() override {
            Stop();
        }

        EUpdateStrategy GetUpdateStrategy(const TFile&, const TFileStat&, const TSharedObjectPtr&) const override {
            return DropOld ? US_DROP_AND_READ : US_READ_AND_SWAP;
        }

        TSharedObjectPtr MakeNewObject(const TFile& file, const TFileStat&, TSharedObjectPtr p, bool) const override {
            if (DropOld) {
                Y_VERIFY(!p, "not dropped before read");
            }

            TFileInput fin(file);
            TString ss = fin.ReadAll();

            Y_VERIFY(!!ss, "%s is empty", file.GetName().data());
            Clog.Write(Sprintf("Read file %s\n", file.GetName().data()));

            return new TStringHolder(ss);
        }
    };

    struct TRegistryTester : TRegistryChecker<TFileReader, TCheckerOpts> {
        TRegistryTester(const TString& file)
            : TRegistryChecker<TFileReader, TCheckerOpts>(file, 1)
        {
        }

        ~TRegistryTester() override {
            Stop();
        }

        void GetNewFileSet(const TFile& file, TFileSet& fs) const override {
            TFileInput fin(file);
            TString line;
            if (!fin.ReadLine(line)) {
                Y_FAIL("Empty registry");
                return;
            }

            ui32 num = FromString<ui32>(line);

            Y_VERIFY(num, "Zero in registry");

            ui32 n = 0;
            while (fin.ReadLine(line)) {
                ++n;
                fs[line];
                if (n % 2) {
                    fs[line].DropOld = true;
                }
            }

            Y_VERIFY(n == num, "Bad registry %u %u", n, num);
            Clog.Write("Read registry\n");
        }
    };

    ui64 AssertObject(TRegistryTester::TItemCheckerPtr ch) {
        Y_VERIFY(!!ch, "Checker is null");

        TString fname = ch->GetMonitorFileName();
        TFileReader::TSharedObjectPtr p = ch->GetObject();

        if (!p) {
            return 0;
        }

        TStringBuf s(p->String);
        TStringBuf l = s.NextTok('\n');

        Y_VERIFY(!!l, "%s is empty", fname.data());

        ui32 i = FromString<ui32>(l);
        ui32 j = 1;
        while (!!s) {
            s.NextTok('\n');
            ++j;
        }

        Y_VERIFY(i == j, "%s invalid %u != %u", fname.data(), i, j);

        return 1;
    }

    ui64 AssertRegistry(TRegistryTester::TSharedObjectPtr obj) {
        static bool initialized = false;

        if (!obj) {
            Y_VERIFY(!initialized, "Registry is null");
            return 0;
        }

        TRegistryTester::TCheckedRegistry reg = *obj;
        initialized = true;

        Y_VERIFY(!reg.empty(), "Registry is empty");

        ui64 res = 0;

        for (TRegistryTester::TCheckedRegistry::const_iterator it = reg.begin(); it != reg.end(); ++it) {
            res += AssertObject(it->second);
        }

        return res;
    }

}

int main(int argc, const char* argv[]) {
    {
        if (argc < 6 || argc < 2 || TStringBuf(argv[1]) == "--help") {
            Clog << argv[0] << " <prefix> <numfiles> <fsize> <upiters> <checkiters>" << Endl;
            exit(0);
        }

        TString prefix = argv[1];
        ui32 numfiles = FromString<ui32>(argv[2]);
        ui32 fsize = FromString<ui32>(argv[3]);
        ui32 iters = FromString<ui32>(argv[4]);
        ui32 chiters = FromString<ui32>(argv[5]);

        TVector<TString> files;

        for (ui32 i = 0; i < numfiles; ++i) {
            files.push_back(Sprintf("%sfile.%u", prefix.data(), i));
        }

        NChecker::TRegistryMutator mut(Sprintf("%s.registry", prefix.data()), fsize, files);
        NChecker::TRegistryTester tester(mut.RegistryFile);
        tester.Start();

        ui64 res = 0;

        for (ui32 i = 0; i < iters; ++i) {
            mut.Update();

            for (ui32 j = 0; j < chiters; ++j) {
                res += NChecker::AssertRegistry(tester.GetObject());
            }
        }

        //        Y_VERIFY(res, "Could not read not even once. Consider this an error.");
    }

    Cout << "OK" << Endl;
}
