#pragma once

#include "mirrors.h"

#include <library/cpp/getopt/small/opt.h>
#include <library/cpp/uri/http_url.h>

#include <util/generic/yexception.h>
#include <util/stream/file.h>

class IMirrors {
public:
    typedef TVector<TString> THosts;
    virtual const char* Check(const char* host) const = 0;
    virtual void GetGroup(const char* host, THosts* result) const = 0;
    virtual int IsMain(const char* host) const = 0;
    virtual int IsSoft(const char* host) const = 0;
    virtual int IsSafe(const char* host) const = 0;
    virtual ~IMirrors() {}

    static TAutoPtr<IMirrors> Load(const TString& fileName, int precharge = 0);
};

class TMirrors : public IMirrors {
private:
    THolder<mirrors> Mirrors;
    bool NeedsRelease;

public:
    TMirrors(const TString& mirrorsInput)
        : NeedsRelease(false)
    {
        Mirrors.Reset(new mirrors());
        if (Mirrors->load(mirrorsInput.data()))
            ythrow yexception() <<  "cannot open '" <<  mirrorsInput <<  "'";
    }

    TMirrors(mirrors * mir)
        : NeedsRelease(true)
    {
        Mirrors.Reset(mir);
    }

    ~TMirrors() override {
        if (NeedsRelease)
            Y_UNUSED(Mirrors.Release());
     }

    const char* Check(const char* host) const override {
        if (host)
            return Mirrors->check(host);
        else
            return nullptr;
    }

    void GetGroup(const char* host, THosts* result) const override {
        result->clear();
        if (host) {
            const sgrp* grp = Mirrors->getgroup(host);
            if (grp) {
                for (size_t j = 0; j < grp->size(); ++j)
                    result->push_back((*grp)[j].name);
            }
        }
    }

    int IsMain(const char* host) const override {
        return Mirrors->isMain(host);
    }
    int IsSoft(const char* host) const override {
        return Mirrors->isSoft(host);
    }
    int IsSafe(const char* host) const override {
        return Mirrors->isSafe(host);
    }
};

class TMirrorsHashed : public IMirrors {
private:
    THolder<mirrors_mapped> Mirrors;
    bool NeedsRelease;

public:
    TMirrorsHashed(const TString& mirrorsInput, int precharge = 0)
        : NeedsRelease(false)
    {
        Mirrors.Reset(new mirrors_mapped(mirrorsInput.data(), precharge));
    }

    TMirrorsHashed(mirrors_mapped * mir)
        : NeedsRelease(true)
    {
        Mirrors.Reset(mir);
    }

    ~TMirrorsHashed() override {
        if (NeedsRelease)
            Y_UNUSED(Mirrors.Release());
    }

    const char* Check(const char* host) const override {
        if (host)
            return Mirrors->check(host);
        else
            return nullptr;
    }

    void GetGroup(const char* host, THosts* result) const override {
        result->clear();
        if (host) {
            psgrp grp = Mirrors->getgroup(host);
            for (size_t j = 0; j < grp.size(); ++j)
                result->push_back(grp[j].name);
        }
    }

    int IsMain(const char* host) const override {
        return Mirrors->isMain(host);
    }
    int IsSoft(const char* host) const override {
        return Mirrors->isSoft(host);
    }
    int IsSafe(const char* host) const override {
        return Mirrors->isSafe(host);
    }
};
