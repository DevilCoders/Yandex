#pragma once

namespace {

template <typename T>
T* GetOrAdd(TCatalogus<T>* catalogus, TString name) {
    typename TCatalogus<T>::iterator it = catalogus->find(name);
    if (it == catalogus->end()) {
        catalogus->push_back(new T(name), name);
        it = catalogus->end();
        it--;
    }
    return *it;
}

template <typename T>
TVector<TString> GetNames(const TCatalogus<T>& catalogus) {
    typedef typename TCatalogus<T>::const_iterator Iterator;
    TVector<TString> result;
    for (Iterator i = catalogus.begin(); i != catalogus.end(); i++) {
        result.push_back((*i)->GetName());
    }
    return result;
}

}

struct TCommandTarget {
    TCommandTarget(TString targetName)
        : TargetName(targetName)
    {
    }

    const TString& GetName() const {
        return TargetName;
    }

    TString TargetName;
    TVector<int> Tasks;
};

struct TCommandWorker {
    typedef TCatalogus<TCommandTarget> TByTargetCatalogus;

    TCommandWorker(TString host)
        : Host(host)
    {
    }

    const TString& GetName() const {
        return Host;
    }

    TVector<TString> GetTargets() const {
        return GetNames(Targets);
    }

    TCommandTarget* GetOrAddCommandTarget(TString targetName) {
        return GetOrAdd(&Targets, targetName);
    }

    TString Host;
    TByTargetCatalogus Targets;
};

struct TCommandForWorkers {
    typedef TCatalogus<TCommandWorker> TByWorkerCatalogus;

    TCommandForWorkers()
        : Workers(new TByWorkerCatalogus())
    {
    }

    TVector<TString> GetWorkers() const {
        return GetNames(*Workers);
    }

    TVector<TString> GetTargetsForWorker(const TString& name) const {
        TByWorkerCatalogus::const_iterator it = Workers->find(name);
        if (it == Workers->end()) {
            ythrow yexception() << "There is no worker " << name;
        }
        return (*it)->GetTargets();
    }

    TCommandWorker* GetOrAddCommandWorker(TString host) {
        return GetOrAdd(&*Workers, host);
    }

    TSimpleSharedPtr<TByWorkerCatalogus> Workers;
};
