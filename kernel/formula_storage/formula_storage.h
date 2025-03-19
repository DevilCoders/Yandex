#pragma once

#include "loader.h"
#include "shared_formulas_adapter/shared_formulas_adapter.h"

#include <kernel/extended_mx_calcer/interface/calcer_exception.h>
#include <kernel/index_mapping/index_mapping.h>
#include <kernel/relevfml/models_archive/models_archive.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/archive/directory_models_archive_reader.h>
#include <library/cpp/archive/models_archive_reader.h>
#include <library/cpp/archive/yarchive.h>
#include <library/cpp/threading/async_task_batch/async_task_batch.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>

#include <util/ysaveload.h>
#include <util/folder/path.h>
#include <util/folder/dirut.h>
#include <util/generic/hash.h>
#include <util/generic/is_in.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/system/filemap.h>
#include <util/system/mutex.h>
#include <util/string/cast.h>

namespace NCommonHelpers {
    template <typename TToPtr, typename TFrom>
    TToPtr DynamicCastHelper(TFrom* calcer) {
        using NExtendedMx::TExtendedRelevCalcerSimple;
        using TXtdWrapper = std::conditional_t<std::is_const<TFrom>::value, const TExtendedRelevCalcerSimple, TExtendedRelevCalcerSimple>;
        if (TToPtr result = dynamic_cast<TToPtr>(calcer)) {
            return result;
        } else if (auto* ec = dynamic_cast<TXtdWrapper*>(calcer)) {
            return dynamic_cast<TToPtr>(ec->Get());
        }

        return nullptr;
    }

    inline TString GetFormulaNameFromPath(const TFsPath& path) {
        return TString{TStringBuf(path.GetName()).RSplitOff('.')};
    }

    inline bool CheckFormulaSuffix(const TString& fmlName) {
        static const TVector<TString> suffixes = {
            TString("info"),
            TString("mnmc"),
            TString("regtree"),
            TString("archive"),
            TString{TStringBuf(NCatboostCalcer::TCatboostCalcer::GetFileExtension()).After('.')},
            TString{TStringBuf(NExtendedMx::TExtendedLoaderFactory::GetExtendedFileExtension()).After('.')}};
        return IsIn(suffixes, TString{TStringBuf(fmlName).RAfter('.')});
    }

} // NCommonHelpers

namespace NFormulaStorageImpl {
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TFormulasStorage - holder of different mxnet formulas
    //   Requirements of TRelevCalcer (if it is not IRelevCalcer):
    //     * should implement static THolder<...> Create(THolder<IRelevCalcer>) construction method
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    constexpr size_t DEFAULT_THREADS_COUNT = 10;

    inline static void MakeBadUploadErrorMessage(TStringStream& log, const TStringBuf fmlName,
                                                 const TString& fmlPath, const TStringBuf reason);

    template <class TRelevCalcer>
    class TFormulasStorage: public ISharedFormulasAdapter {
    public:
        using TCalcerType = TRelevCalcer;

    private:
        using TRelevCalcerPtr = TAtomicSharedPtr<TCalcerType>;

    private:
        bool TryToAddFormula(const TStringBuf name, const TString& path);
        void RegisterFmlMd5(const TStringBuf name, NThreading::TFuture<TString> md5);
        bool RegisterFmlID(const TStringBuf name);
        bool TryToRemoveFromFmlIDStorage(const TRelevCalcerPtr& fmlPtr);
        void AddFormulasFromDirectory(const TFsPath& path, const bool recursive,
                                      const size_t depth, const bool checkSuffix, const size_t threadsCount = DEFAULT_THREADS_COUNT);

        void AddFormulasFromDirectory(const TFsPath& path, const bool recursive,
                                      const size_t depth, const bool checkSuffix, TAsyncTaskBatch& taskBatch);

        TAtomicSharedPtr<IThreadPool> CreateMd5ThreadPool();
        void FinalizeMd5ThreadPool(TAtomicSharedPtr<IThreadPool> md5Pool);

        using TStorage = THashMap<TString, TRelevCalcerPtr>;
        TVector<THolder<TFileMap>> MappedArchives;
        TStorage Storage;
        TStorage FmlIDStorage;
        bool IsDebug = false;
        TStringStream ErrorLog;
        TMutex Mutex; // protect data from thread race when add new formulas

        IThreadPool* Md5Pool = nullptr;

        using TModelDigestStorage = THashMap<TString, NThreading::TFuture<TString>>;
        TModelDigestStorage FmlMd5;

        TFormulasStorage(const TFormulasStorage&);
        TFormulasStorage& operator=(const TFormulasStorage&);

    private:
        class TLoader {
        public:
            using TModelDigestCalculator = std::function<TString ()>;
            using TModelDigestCalculatorStorage = THashMap<TString, TModelDigestCalculator>;
        private:
            static const size_t Md5BufSize = 33;

            TStorage& ModelsRef;
            TModelDigestCalculatorStorage& DigestsRef;
            TStringStream& ErrorLog;
            const TFsPath& Path;

        private:
            template <typename TGenericMn>
            bool TryToWriteMnInfo(TRelevCalcer* relevCalcer, const TStringBuf name, const TFsPath& path) {
                if (auto genericMnRawPtr = dynamic_cast<TGenericMn*>(relevCalcer)) {
                    NModelsArchive::WriteMatrixnetInfo(*genericMnRawPtr, name, path.GetPath());
                    DigestsRef[name] = [genericMnRawPtr] {
                        return genericMnRawPtr->MD5();
                    };
                    return true;
                }
                return false;
            }

            bool WriteGenericModelInfo(const TStringBuf name, const TBlob& blob) {
                TStringBuf area(blob.AsCharPtr(), blob.Size());
                DigestsRef[name] = [area] {
                    return MD5::Calc(area);
                };
                return true;
            }

        public:
            TLoader(TStorage& modelsRef, TModelDigestCalculatorStorage& digestsRef,
                TStringStream& errlog, const TFsPath& path)
                : ModelsRef(modelsRef)
                , DigestsRef(digestsRef)
                , ErrorLog(errlog)
                , Path(path)
            {
            }

            void operator()(const IModelsArchiveReader& reader, const TStringBuf key) {
                try {
                    const TBlob blob = reader.BlobByKey(key);
                    THolder<TRelevCalcer> holder;
                    if (key.EndsWith(".info")) {
                        holder = CreateRelevCalcerPtr<TRelevCalcer>(new NMatrixnet::TMnSseInfo(blob.Data(), blob.Length()));
                    } else if (key.EndsWith(".mnmc")) {
                        holder = CreateRelevCalcerPtr<TRelevCalcer>(new NMatrixnet::TMnMultiCateg(blob.Data(), blob.Length()));
                    } else {
                        TMemoryInput inp(blob.AsCharPtr(), blob.Size());
                        holder = LoadFormula<TRelevCalcer>(key, &inp);
                    }
                    const TString name = NMatrixnet::ModelNameFromFilePath(key);
                    TryToWriteMnInfo<NMatrixnet::TMnSseInfo>(holder.Get(), name, Path) || TryToWriteMnInfo<NMatrixnet::TMnMultiCateg>(holder.Get(), name, Path) || WriteGenericModelInfo(name, blob);

                    ModelsRef[name].Reset(holder.Release());
                } catch (const yexception& ex) {
                    MakeBadUploadErrorMessage(ErrorLog, key, Path, TString("unexpected error: ") + ex.what());
                }
            }
        };

    public:
        TFormulasStorage() = default;
        TFormulasStorage(const bool debug);

        ~TFormulasStorage() override;

        bool AddFormula(const TStringBuf name, const TString& path);
        bool AddFormula(const TString& path);
        void AddFormulasFromDirectory(const TFsPath& path, const size_t threadsCount = DEFAULT_THREADS_COUNT);
        void AddFormulasFromDirectoryRecursive(const TFsPath& path, const size_t threadsCount = DEFAULT_THREADS_COUNT, const size_t depth = 10);
        void AddValidFormulasFromDirectory(const TFsPath& path, const size_t threadsCount = DEFAULT_THREADS_COUNT);
        void AddValidFormulasFromDirectoryRecursive(const TFsPath& path, const size_t threadsCount = DEFAULT_THREADS_COUNT, const size_t depth = 10);
        bool AddFormulasFromArchiveReader(const TFsPath& path, const IModelsArchiveReader&, const TFsPath& subDir = "");
        bool AddFormulasFromArchive(const TFsPath& path, const TFsPath& subDir = "", bool lockMemory = false);

        void Clear();
        bool RemoveFormula(const TStringBuf name);
        TString GetErrorLog(const bool clear = false);
        void Finalize();

        bool CheckFormulaUsed(const TStringBuf name) const;
        bool Empty() const;
        size_t Size() const;
        double GetFormulaValue(const TStringBuf name, const float* factors) const;
        const TRelevCalcer* GetFormula(const TStringBuf name) const;
        const TRelevCalcer* GetFormulaByName(const TStringBuf name) const;
        const TRelevCalcer* GetFormulaByFmlID(const TStringBuf fmlID) const;
        ISharedFormulasAdapter::TBaseCalcerPtr GetSharedFormula(const TStringBuf name) const override;

        template <class T>
        bool GetFormulaValues(const TStringBuf name, const T& factors, TVector<double>& results) const;
        void GetDefinedFormulaNames(TVector<TString>& names, const bool sorted = false) const;
        bool GetFormulaCategsByName(const TStringBuf name, const TVector<float>& factors, TVector<double>& categs, TVector<double>& categValues) const;
        bool GetMaxCategValueByName(const TStringBuf name, const TVector<float>& factors, double& maxCategValue) const;
        const TString& GetFormulaMd5(const TStringBuf name) const;

        void WaitAsyncJobs() {
            for (const auto& md5: FmlMd5) {
                md5.second.Wait();
            }
        }
    };

    template <class TRelevCalcer>
    TFormulasStorage<TRelevCalcer>::TFormulasStorage(const bool debug)
        : IsDebug(debug)
    {
    }

    template <class TRelevCalcer>
    TFormulasStorage<TRelevCalcer>::~TFormulasStorage() {
        WaitAsyncJobs();
        Clear();
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::CheckFormulaUsed(const TStringBuf name) const {
        with_lock(Mutex) {
            return Storage.contains(name);
        }
    }

    inline static void MakeBadUploadErrorMessage(TStringStream& log, const TStringBuf fmlName,
                                                 const TString& fmlPath, const TStringBuf reason) {
        log << "WARNING: can not add model " << fmlName << " from " << fmlPath
            << ": " << reason << Endl;
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::TryToAddFormula(const TStringBuf name, const TString& path) {
        bool used = CheckFormulaUsed(name);
        if (!used) {
            THolder<TRelevCalcer> mn;
            try {
                TFileInput in(path);
                mn.Reset(LoadFormula<TRelevCalcer>(path, &in));
            } catch (const TUnknownExtensionException&) {
                return false;
            } catch (const yexception& exp) {
                if (IsDebug) {
                    MakeBadUploadErrorMessage(ErrorLog, name, path, TString("unexpected error: ") + exp.what());
                }
                return false;
            }

            with_lock(Mutex) {
                auto md5Future = Md5Pool ?
                    NThreading::Async([path] { return MD5::File(path); }, *Md5Pool) :
                    NThreading::MakeFuture(MD5::File(path));

                RegisterFmlMd5(name, std::move(md5Future));
                Storage[name].Reset(mn.Release());
                RegisterFmlID(name);
           }
        } else if (IsDebug) {
            MakeBadUploadErrorMessage(ErrorLog, name, path, "model with such name is already present in storage");
        }
        return !used;
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::RegisterFmlMd5(const TStringBuf name, NThreading::TFuture<TString> md5) {
        FmlMd5.emplace(ToString(name), std::move(md5));
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::RegisterFmlID(const TStringBuf name) {
        if (TRelevCalcerPtr ptr = Storage[name]) {
            if (const NMatrixnet::TModelInfo* info = ptr->GetInfo()) {
                if (const TString* fmlID = info->FindPtr("formula-id")) {
                    FmlIDStorage[*fmlID] = ptr;
                    return true;
                }
            }
        }
        return false;
    }

    template <class TRelevCalcer>
    const TString& TFormulasStorage<TRelevCalcer>::GetFormulaMd5(const TStringBuf name) const {
        static const TString deflt;
        const auto* md5Future = FmlMd5.FindPtr(name);
        if (md5Future && md5Future->HasValue()) {
            return md5Future->GetValue();
        }
        return deflt;
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::AddFormula(const TStringBuf name, const TString& path) {
        if (path.EndsWith(".archive")) {
            return AddFormulasFromArchive(path);
        }
        return TryToAddFormula(name, path);
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::AddFormula(const TString& path) {
        return AddFormula(NCommonHelpers::GetFormulaNameFromPath(path), path);
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::AddFormulasFromDirectory(const TFsPath& path, const bool recursive,
                                                                  const size_t depth, const bool checkSuffix, TAsyncTaskBatch& taskBatch) {
        if (!path.IsDirectory()) {
            return;
        }
        if (recursive && !depth) {
            return;
        }

        TVector<TFsPath> children;
        path.List(children);
        for (TVector<TFsPath>::const_iterator it = children.begin(); it != children.end(); ++it) {
            if (recursive && it->IsDirectory() && !it->IsSymlink()) {
                AddFormulasFromDirectory(it->GetPath(), recursive, depth - 1, checkSuffix, taskBatch);
            } else if (it->IsFile()) {
                if (checkSuffix && !NCommonHelpers::CheckFormulaSuffix(it->GetPath())) {
                    continue;
                }
                with_lock(Mutex) {
                    TString formulaName = NCommonHelpers::GetFormulaNameFromPath(*it);
                    TString path = it->GetPath();
                    taskBatch.Add([this, formulaName, path]{
                        AddFormula(formulaName, path);
                    });
                }
            }
        }
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::AddFormulasFromDirectory(const TFsPath& path, const bool recursive,
                                                                  const size_t depth, const bool checkSuffix, const size_t threadsCount) {
        auto md5Pool = CreateMd5ThreadPool();
        THolder<IThreadPool> queue = CreateThreadPool(threadsCount, 0, "FormulaAddQueue");
        TAsyncTaskBatch taskBatch(queue.Get());
        AddFormulasFromDirectory(path, recursive, depth, checkSuffix, taskBatch);
        taskBatch.WaitAllAndProcessNotStarted();
        FinalizeMd5ThreadPool(std::move(md5Pool));
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::AddFormulasFromDirectory(const TFsPath& path, const size_t threadsCount) {
        AddFormulasFromDirectory(path, false, 0, false, threadsCount);
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::AddFormulasFromDirectoryRecursive(const TFsPath& path, const size_t threadsCount, const size_t depth) {
        AddFormulasFromDirectory(path, true, depth, false, threadsCount);
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::AddValidFormulasFromDirectory(const TFsPath& path, const size_t threadsCount) {
        AddFormulasFromDirectory(path, false, 0, true, threadsCount);
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::AddValidFormulasFromDirectoryRecursive(const TFsPath& path, const size_t threadsCount, const size_t depth) {
        AddFormulasFromDirectory(path, true, depth, true, threadsCount);
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::AddFormulasFromArchiveReader(const TFsPath& path,
                                                                      const IModelsArchiveReader& archiveReader,
                                                                      const TFsPath& subDir) {
        auto md5Pool = CreateMd5ThreadPool();

        TStorage models;
        typename TLoader::TModelDigestCalculatorStorage digests;
        NModelsArchive::Load(archiveReader, TLoader(models, digests, ErrorLog, path), subDir.GetPath());

        size_t inserted = 0;
        for (auto it : models) {
            const TString& fmlName = it.first;
            if (!CheckFormulaUsed(fmlName)) {
                with_lock(Mutex) {
                    Storage[fmlName].Swap(it.second);

                    auto md5Calculator = digests[fmlName];
                    auto md5Future = Md5Pool ?
                        NThreading::Async([md5Calculator] { return md5Calculator(); }, *Md5Pool) :
                        NThreading::MakeFuture(md5Calculator());
                    RegisterFmlMd5(fmlName, std::move(md5Future));
                }
                ++inserted;
            } else if (IsDebug) {
                MakeBadUploadErrorMessage(ErrorLog, it.first, path, "model with such name is already present in storage");
            }
        }
        FinalizeMd5ThreadPool(std::move(md5Pool));
        return models.size() == inserted;
    }

    template <class TRelevCalcer>
    TAtomicSharedPtr<IThreadPool> TFormulasStorage<TRelevCalcer>::CreateMd5ThreadPool() {
        TAtomicSharedPtr<IThreadPool> md5Pool = CreateThreadPool(10, 0, "FormulasStorage");
        with_lock(Mutex) {
            Md5Pool = md5Pool.Get();
        }

        return md5Pool;
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::FinalizeMd5ThreadPool(TAtomicSharedPtr<IThreadPool> md5Pool) {
        with_lock(Mutex) {
            Md5Pool = nullptr;
        }

        SystemThreadFactory()->Run([md5PoolMoved = std::move(md5Pool)]() mutable {
            md5PoolMoved.Reset();
        });
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::AddFormulasFromArchive(const TFsPath& path, const TFsPath& subDir, bool lockMemory) {
        if (IsDir(path.GetPath())) {
            const TDirectoryModelsArchiveReader archiveReader(path.GetPath(), lockMemory);
            return AddFormulasFromArchiveReader(path, archiveReader, subDir);
        } else {
            THolder<TFileMap> mappedArchiveHolder;
            if (const TMemoryMap* memMap = GetMappedIndexFile(path.GetPath())) {
                mappedArchiveHolder.Reset(new TFileMap(*memMap));
            } else {
                mappedArchiveHolder.Reset(new TFileMap(path.GetPath()));
            }

            try {
                mappedArchiveHolder->Map(0, mappedArchiveHolder->Length());
            } catch (const yexception& ex) {
                if (IsDebug) {
                    ErrorLog << "mapping while loading archive for fml storage failed, path: " << path
                             << " exception: " << ex.what() << Endl;
                }
                return false;
            }

            TBlob localBlob;
            with_lock(Mutex) {
                MappedArchives.push_back(std::move(mappedArchiveHolder));
                localBlob = TBlob::NoCopy(MappedArchives.back()->Ptr(), MappedArchives.back()->MappedSize());
            }

            TArchiveReader archiveReader(localBlob);

            return AddFormulasFromArchiveReader(path, archiveReader, subDir);
        }
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::Empty() const {
        return Storage.empty();
    }

    template <class TRelevCalcer>
    size_t TFormulasStorage<TRelevCalcer>::Size() const {
        return Storage.size();
    }

    template <class TRelevCalcer>
    double TFormulasStorage<TRelevCalcer>::GetFormulaValue(const TStringBuf name, const float* factors) const {
        typename TStorage::const_iterator it = Storage.find(name);
        if (it != Storage.end()) {
            return it->second->DoCalcRelev(factors);
        }
        return -1;
    }

    template <class TRelevCalcer>
    template <class T>
    bool TFormulasStorage<TRelevCalcer>::GetFormulaValues(const TStringBuf name, const T& factors,
                                                          TVector<double>& results) const {
        typename TStorage::const_iterator it = Storage.find(name);
        if (it != Storage.end()) {
            it->second->CalcRelevs(factors, results);
            return true;
        }
        return false;
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::Clear() {
        TGuard<TMutex> guard(Mutex);
        FmlIDStorage.clear();
        Storage.clear();
        FmlMd5.clear();
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::TryToRemoveFromFmlIDStorage(const TRelevCalcerPtr& fmlPtr) {
        if (fmlPtr) {
            if (const NMatrixnet::TModelInfo* info = fmlPtr->GetInfo()) {
                if (const TString* fmlID = info->FindPtr("formula-id")) {
                    FmlIDStorage.erase(*fmlID);
                    return true;
                }
            }
        }
        return false;
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::RemoveFormula(const TStringBuf name) {
        TGuard<TMutex> guard(Mutex);
        typename TStorage::iterator it = Storage.find(name);
        if (it != Storage.end()) {
            TryToRemoveFromFmlIDStorage(it->second);
            Storage.erase(it);
            FmlMd5.erase(name);
            return true;
        }
        return false;
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::GetDefinedFormulaNames(TVector<TString>& names, const bool sorted) const {
        names.clear();

        with_lock(Mutex) {
            names.reserve(Storage.size());
            for (typename TStorage::const_iterator it = Storage.begin(); it != Storage.end(); ++it)
                names.push_back(it->first);
        }

        if (sorted) {
            Sort(names.begin(), names.end());
        }
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::GetFormulaCategsByName(const TStringBuf name,
                                                                const TVector<float>& factors, TVector<double>& categs,
                                                                TVector<double>& categValues) const {
        for (typename TStorage::const_iterator it = Storage.begin(); it != Storage.end(); ++it) {
            if (it->first == name) {
                const NMatrixnet::TMnMultiCateg* formula =
                    NCommonHelpers::DynamicCastHelper<const NMatrixnet::TMnMultiCateg*>(it->second.Get());

                if (!formula) {
                    return false;
                }
                categValues.assign(formula->CategValues().begin(), formula->CategValues().end());
                categs.resize(categValues.size());
                formula->CalcCategs(TVector<TVector<float>>(1, factors), categs.data());
                return true;
            }
        }
        return false;
    }

    template <class TRelevCalcer>
    bool TFormulasStorage<TRelevCalcer>::GetMaxCategValueByName(const TStringBuf name,
                                                                const TVector<float>& factors, double& maxCategValue) const {
        TVector<double> categs;
        TVector<double> categValues;
        if (GetFormulaCategsByName(name, factors, categs, categValues)) {
            double maxWeight = 0.0;
            for (size_t i = 0; i < categs.size(); ++i) {
                if (maxWeight < categs[i]) {
                    maxWeight = categs[i];
                    maxCategValue = categValues[i];
                }
            }
            return true;
        }
        return false;
    }

    template <class TRelevCalcer>
    const TRelevCalcer* TFormulasStorage<TRelevCalcer>::GetFormula(const TStringBuf name) const {
        auto* formulaPtr = GetFormulaByName(name);
        if (!formulaPtr) {
            return GetFormulaByFmlID(name);
        }
        return formulaPtr;
    }

    template <class TRelevCalcer>
    const TRelevCalcer* TFormulasStorage<TRelevCalcer>::GetFormulaByName(const TStringBuf name) const {
        auto* formulaPtr = Storage.FindPtr(name);
        return !!formulaPtr ? formulaPtr->Get() : nullptr;
    }

    template <class TRelevCalcer>
    const TRelevCalcer* TFormulasStorage<TRelevCalcer>::GetFormulaByFmlID(const TStringBuf fmlID) const {
        auto* formulaPtr = FmlIDStorage.FindPtr(fmlID);
        return !!formulaPtr ? formulaPtr->Get() : nullptr;
    }

    template <class TRelevCalcer>
    ISharedFormulasAdapter::TBaseCalcerPtr TFormulasStorage<TRelevCalcer>::GetSharedFormula(const TStringBuf name) const {
        auto* formulaPtr = Storage.FindPtr(name);
        return formulaPtr ? *formulaPtr : TRelevCalcerPtr(nullptr);
    }

    template <class TRelevCalcer>
    TString TFormulasStorage<TRelevCalcer>::GetErrorLog(const bool clear) {
        TGuard<TMutex> guard(Mutex);
        if (IsDebug) {
            TString log = ErrorLog.Str();
            if (clear) {
                ErrorLog.Clear();
            }
            return log;
        }
        ythrow yexception() << "Debug mode is switched off" << Endl;
    }

    template <class TRelevCalcer>
    void TFormulasStorage<TRelevCalcer>::Finalize() {
        TMap<TString, TString> invalidFormulaToReason;

        with_lock(Mutex) {
            for (const auto& formula : Storage) {
                try {
                    formula.second->Initialize(this);
                } catch (yexception& exp) {
                    if (TCalcerException* calcerExp = dynamic_cast<TCalcerException*>(&exp)) {
                        calcerExp->AddCalcerToErroredChain(formula.first);
                    }
                    invalidFormulaToReason[formula.first] = exp.what();
                }
            }
        }

        for (const auto& invalidFormulaAndReason : invalidFormulaToReason) {
            if (IsDebug) {
                ErrorLog << "The formula " << invalidFormulaAndReason.first << " is removed from formulas' storage: "
                         << "initialization failed. " << invalidFormulaAndReason.second << Endl;
            }
            RemoveFormula(invalidFormulaAndReason.first);
        }
    }

} // NFormulaStorageImpl

// this is a typedef for forward declarations
class TFormulasStorage: public NFormulaStorageImpl::TFormulasStorage<NMatrixnet::IRelevCalcer> {
    using TParent = NFormulaStorageImpl::TFormulasStorage<NMatrixnet::IRelevCalcer>;

public:
    using TParent::TParent;
};
