#include "persistent.h"

#include <library/cpp/logger/global/global.h>

#include <util/digest/fnv.h>
#include <util/folder/dirut.h>
#include <util/generic/hash.h>
#include <util/system/filemap.h>
#include <util/system/fs.h>
#include <util/system/rwlock.h>

namespace {
    struct TElement {
        ui64 NameHash;
        TAtomic Value;
    };

    template <class T>
    class TMutableFileMappedArray {
    public:
        TMutableFileMappedArray(const TString& filename)
            : Map(filename, TFileMap::oRdWr)
        {
            if (Map.Length() % sizeof(T)) {
                throw yexception() << "incorrect size of " << filename;
            }
            Map.Map(0, Map.Length());

            Begin = reinterpret_cast<T*>(Map.Ptr());
            Size = Map.MappedSize() / sizeof(T);
            End = Begin + Size;
        }

        inline T* begin() const {
            return Begin;
        }
        inline T* end() const {
            return End;
        }
        inline size_t size() const {
            return Size;
        }
        inline bool empty() const {
            return Size == 0;
        }
        inline T& operator[](size_t pos) const {
            Y_ASSERT(pos < size());
            return Begin[pos];
        }

    private:
        TFileMap Map;

        T* Begin = nullptr;
        T* End = nullptr;
        size_t Size = 0;
    };

    using TPersistentMetricBackend = TMutableFileMappedArray<TElement>;
    using TPersistentMetricBackendPtr = TAtomicSharedPtr<TPersistentMetricBackend>;

    class TPersistentMetricStorage {
    public:
        struct TMetric {
            TPersistentMetricBackendPtr Backend = nullptr;
            TAtomic* Value = nullptr;
        };
    public:
        void SetStorage(const TString& filename) {
            TWriteGuard guard(Lock);
            if (FileName == filename) {
                return;
            }

            if (filename) {
                if (!NFs::Exists(filename)) {
                    ZeroMetrics();
                    Serialize(filename);
                }

                Deserialize(filename);
                FileName = filename;
            }
        }

        TAtomicBase Get(ui64 id) const {
            TReadGuard guard(Lock);
            auto metric = GetMetric(id);
            return metric ? AtomicGet(*metric) : 0;
        }
        void Set(ui64 id, TAtomicBase value) const {
            TReadGuard guard(Lock);
            auto metric = GetMetric(id);
            if (metric) {
                AtomicSet(*metric, value);
            }
        }
        void Add(ui64 id, TAtomicBase value) const {
            TReadGuard guard(Lock);
            auto metric = GetMetric(id);
            if (metric) {
                AtomicAdd(*metric, value);
            }
        }

        void Register(ui64 id) {
            {
                TReadGuard guard(Lock);
                if (Metrics.contains(id)) {
                    return;
                }
            }
            {
                TWriteGuard guard(Lock);
                AddMetric(id);

                if (FileName) {
                    const TString& tmp = FileName + ".tmp";
                    Serialize(tmp);
                    Drop();
                    CHECK_WITH_LOG(NFs::Rename(tmp, FileName));

                    Deserialize(FileName);
                }
            }
        }
        void Deregister(ui64 /*id*/) {
        }

    private:
        void AddMetric(ui64 id) {
            Metrics[id];
        }
        TAtomic* GetMetric(ui64 id) const {
            auto metric = Metrics.find(id);
            CHECK_WITH_LOG(metric != Metrics.end());
            return metric->second.Value;
        }
        void ZeroMetrics() const {
            for (auto&& metric : Metrics) {
                AtomicSet(*metric.second.Value, 0);
            }
        }
        void Serialize(const TString& filename) const {
            const size_t size = Metrics.size() * sizeof(TElement);
            TFile(filename, CreateNew).Resize(size);
            TPersistentMetricBackend backend(filename);

            size_t count = 0;
            for (auto&& metric: Metrics) {
                TElement& element = backend[count];
                element.NameHash = metric.first;
                element.Value = metric.second.Value ? AtomicGet(*metric.second.Value) : 0;

                count++;
            }
        }
        void Deserialize(const TString& filename) {
            Backend = MakeAtomicShared<TPersistentMetricBackend>(filename);
            for (auto&& element: *Backend) {
                TMetric& metric = Metrics[element.NameHash];
                metric.Backend = Backend;
                metric.Value = &element.Value;
            }
        }
        void Drop() {
            Metrics.clear();
            Backend.Drop();
        }

    private:
        THashMap<ui64, TMetric> Metrics;

        TString FileName;
        TPersistentMetricBackendPtr Backend;

        mutable TRWMutex Lock;
    };
}

TPersistentMetric::TPersistentMetric(const TString& name)
    : Name(name)
    , Id(FnvHash<ui64>(name))
    , Session(JoinMetricName(name, "Session"))
{
    Singleton<TPersistentMetricStorage>()->Register(Id);
}

TPersistentMetric::~TPersistentMetric() {
    Singleton<TPersistentMetricStorage>()->Deregister(Id);
}

void TPersistentMetric::Add(TAtomicBase value) {
    Singleton<TPersistentMetricStorage>()->Add(Id, value);
    Session.Add(value);
}

void TPersistentMetric::Set(TAtomicBase value) {
    Singleton<TPersistentMetricStorage>()->Set(Id, value);
    Session.Set(value);
}

TAtomicBase TPersistentMetric::Get() const {
    return Singleton<TPersistentMetricStorage>()->Get(Id);
}

void TPersistentMetric::Register(TMetrics& metrics) const {
    TMetricGetterPtr getter = MakeIntrusive<TConstMethodCaller<const TPersistentMetric>>(this, &TPersistentMetric::Get);
    metrics.RegisterMetric(Name, getter);
    Session.Register(metrics);
}

void TPersistentMetric::Deregister(TMetrics& metrics) const {
    Session.Deregister(metrics);
    metrics.DeRegisterMetric(Name);
}

void SetPersistentMetricsStorage(const TString& filename) {
    Singleton<TPersistentMetricStorage>()->SetStorage(filename);
}
