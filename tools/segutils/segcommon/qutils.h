#pragma once

#include <util/generic/vector.h>
#include <util/folder/path.h>
#include <kernel/hosts/owner/owner.h>

namespace NSegutils {

struct TStat {
private:
    ui32 Tp_;
    ui32 Fp_;
    ui32 Fn_;
    ui32 Total_;
    bool PrUndef_;
public:
    TStat() : Tp_(), Fp_(), Fn_(), Total_(), PrUndef_() {
    }

    void AddTp(ui32 tp=1) {
        Tp_+=tp;
        Total_+=tp;
    }

    void AddFp(ui32 fp=1) {
        Fp_+=fp;
    }

    void AddFn(ui32 fn=1) {
        Fn_+=fn;
        Total_ += fn;
    }

    void SetPrUndef() {
        PrUndef_ = true;
    }

    ui32 TpFn() const {
        return Tp_ + Fn_;
    }

    ui32 TpFp() const {
        return Tp_ + Fp_;
    }

    ui32 Tp() const {
        return Tp_;
    }

    ui32 Fp() const {
        return Fp_;
    }

    ui32 Fn() const {
        return Fn_;
    }

    ui32 Total() const {
        return Total_;
    }

    bool PrUndef() const {
        return PrUndef_;
    }

    float Err() const {
        return Total_ ? float(Fp_)/Total_ : 0;
    }

    float Pr() const {
        return Tp_ || Fp_ ? float(Tp_)/(Tp_ + Fp_) : 1;
    }

    float Re() const {
        return Tp_ || Fn_ ? float(Tp_)/(Tp_ + Fn_) : 1;
    }

    float F1() const {
        return Pr() || Re() ? 2*Pr()*Re()/(Pr()+Re()) : 0;
    }

    operator ui32() const {
        return Total_;
    }
};
typedef TVector<TStat> TStats;

TOwnerCanonizer CreateAndInitCanonizer(const TString& urlRulesFileName);

}
