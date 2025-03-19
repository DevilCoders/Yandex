#pragma once

#include <yweb/fleur/util/metrics/metrics.h>

class TPersistentMetric {
public:
    TPersistentMetric(const TString& name);
    ~TPersistentMetric();

    const TString& GetName() const noexcept {
        return Name;
    }
    const TString& GetSessionName() const noexcept {
        return Session.GetName();
    }

    inline void Inc() {
        Add(1);
    }
    inline void Dec() {
        Add(-1);
    }

    void Add(TAtomicBase value);
    void Set(TAtomicBase value);
    TAtomicBase Get() const;

    void Register(TMetrics& metrics) const;
    void Deregister(TMetrics& metrics) const;

private:
    const TString Name;
    const ui64 Id;

    TOrangeMetric Session;
};

void SetPersistentMetricsStorage(const TString& filename);
