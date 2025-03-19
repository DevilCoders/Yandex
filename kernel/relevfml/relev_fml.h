#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

#include <library/cpp/string_utils/ascii_encode/ascii_encode.h>

template<typename TCmd>
inline TCmd GetMaxBit() noexcept {
    return 1 << (sizeof(TCmd) * 8 - 1);
}

template<typename TCmd>
struct SRelevanceFormulaBase
{
    TVector<TCmd> cmds;
    TVector<float> params;

    void Save(IOutputStream *out) const {
        ::Save(out, cmds);
        ::Save(out, params);
    }

    void Load(IInputStream *in) {
        ::Load(in, cmds);
        ::Load(in, params);
    }

    bool IsEmpty() const { return cmds.empty(); }

    void UsedFactors(TSet<ui32>& result) const {
        const TCmd maxBit = GetMaxBit<TCmd>();
        for (typename TVector<TCmd>::const_iterator i = cmds.begin(); i != cmds.end(); ++i)
            if (*i != maxBit)
                result.insert((*i & (maxBit - 1)) - 1);
    }

    float Calc(const float *fFactors) const
    {
        const TCmd maxBit = GetMaxBit<TCmd>();
        const TCmd *pCmds = cmds.data(), *pCmdsEnd = pCmds + cmds.size();
        int nParam = 0;
        double fRes = 0, fAdd = 1;
        for (; pCmds < pCmdsEnd; ++pCmds) {
            TCmd c = *pCmds;
            if (c != maxBit) {
                int nFactor = (c & (maxBit - 1)) - 1;
                fAdd *= fFactors[nFactor];
            }
            if (c & maxBit) {
                fRes += fAdd * params[nParam++];
                fAdd = 1;
            }
        }
        return static_cast<float>(fRes);
    }

    template<class T>
    void AddSlag(const T& facs, float fWeight)
    {
        const TCmd maxBit = GetMaxBit<TCmd>();
        if (facs.empty())
            cmds.push_back(maxBit);
        else {
            for (typename T::const_iterator i = facs.begin(); i != facs.end(); ++i) {
                TCmd c = (TCmd)(*i + 1);
                if (i + 1 == facs.end())
                    c |= maxBit;
                cmds.push_back(c);
            }
        }
        params.push_back(fWeight);
    }

    template<class T1, class T2>
    void GetFormula(T1* pParams, T2* pWeights) const {
        const TCmd maxBit = GetMaxBit<TCmd>();
        int n = 0;
        const TCmd *pCmds = cmds.data(), *pCmdsEnd = pCmds + cmds.size();
        typename T1::value_type cur;
        for(; pCmds < pCmdsEnd; ++pCmds) {
            TCmd c = *pCmds;
            int nFactor = (c & (maxBit - 1)) - 1;
            if (c != maxBit)
                cur.push_back(nFactor);
            if (c & maxBit) {
                pWeights->push_back(params[n++]);
                pParams->push_back(cur);
                cur.resize(0);
            }
        }
    }
};

struct SRelevanceFormula : public SRelevanceFormulaBase<ui16> {
};

template<class TCmd>
TString Encode(const SRelevanceFormulaBase<TCmd> &f) {
    TVector<unsigned char> buf;

    int nCmdsSize = (int)(f.cmds.size() * sizeof(f.cmds[0]));
    int nFloatDataSize = (int)(f.params.size() * sizeof(f.params[0]));

    buf.reserve(nCmdsSize + 1 + nFloatDataSize);

    const char* pszCmds = (const char*)f.cmds.data();
    buf.insert(buf.end(), pszCmds, pszCmds + nCmdsSize);

    for (size_t i = 0; i < sizeof(TCmd); ++i)
        buf.push_back((unsigned char)0);

    const char *pszFloatData = (const char*)f.params.data();
    buf.insert(buf.end(), pszFloatData, pszFloatData + nFloatDataSize);
    TString out;
    EncodeAscii(buf, &out);
    return out;
}

template<class TCmd>
void Decode(SRelevanceFormulaBase<TCmd>* pRes, const TStringBuf sz, int nFactorCount) {
    *pRes = SRelevanceFormulaBase<TCmd>();
    TVector<unsigned char> buf;
    if (!DecodeAscii(sz, &buf))
        ythrow yexception() << "cannot decode formula: decode ascii";

    int nParams = 0, nPos = 0;
    bool bHasZero = false;
    for (int nLen = (int)buf.size(); nPos < nLen;) {
        TCmd c = *(const TCmd*)(&buf[nPos]);
        nPos += sizeof(TCmd);
        if (c == 0) {
            bHasZero = true;
            break;
        }
        if (c & GetMaxBit<TCmd>())
            ++nParams;
        if (c != GetMaxBit<TCmd>()) {
            int nFactor = (c & (GetMaxBit<TCmd>() - 1)) - 1;
            if (nFactor < 0 || nFactor >= nFactorCount)
                ythrow yexception() << "cannot decode formula: too big number factor '" <<  nFactor << "'";
        }
        pRes->cmds.push_back(c);
    }
    if (!bHasZero)
        ythrow yexception() << "cannot decode formula: has no zero";

    pRes->params.resize(nParams);
    if (nParams) {
        int nFloatDataSize = (int)pRes->params.size() * sizeof(pRes->params[0]);
        if (nPos + nFloatDataSize != (int)buf.size())
            ythrow yexception() << "cannot decode formula: buffer size";
        memcpy(&pRes->params[0], &buf[nPos], nFloatDataSize);
    }
}

template<class T1, class T2>
void ConvertFormula(const SRelevanceFormulaBase<T1>& source, SRelevanceFormulaBase<T2>* dest) {
    TVector< TVector<size_t> > params;
    TVector<float> weights;

    source.GetFormula(&params, &weights);

    *dest = SRelevanceFormulaBase<T2>();

    for (size_t i = 0; i < weights.size(); ++i) {
        dest->AddSlag(params[i], weights[i]);
    }
}

struct TPolynomDescr {
    SRelevanceFormula Descr;
    typedef TMap<TString, TString> TInfo;
    TInfo Info;

    void Save(IOutputStream *out) const {
        ::Save(out, Descr);
        ::Save(out, Info);
    }

    void Load(IInputStream *in) {
        ::Load(in, Descr);
        ::Load(in, Info);
    }
};

struct TPolynomDescrStatic {
    /// Polynomial for calculating relevance.
    typedef float (*TFormula)(const float*);

    /// Polynomial for calculating relevance with SSE instructions set.
    typedef void (*TFormulaSSE)(const float* const*, float*);

    TFormula Fml;       //!< Simple formula for calculating relevance of single document.
    TFormulaSSE FmlSSE; //!< SSE optimized formula for calculating relevance of array of documents.
    const SRelevanceFormula* Descr; //!< Description of the formula.

    const char* Name; //!< @Deprecated

    TPolynomDescr::TInfo Info;

    TPolynomDescrStatic()
        : Fml(nullptr)
        , FmlSSE(nullptr)
        , Descr(nullptr)
        , Name(nullptr)
    {
    }

    explicit TPolynomDescrStatic(const SRelevanceFormula* descr)
        : Fml(nullptr)
        , FmlSSE(nullptr)
        , Descr(descr)
        , Name(nullptr)
    {
    }

    explicit TPolynomDescrStatic(const TPolynomDescr &descr)
        : Fml(nullptr)
        , FmlSSE(nullptr)
        , Descr(&descr.Descr)
        , Name(nullptr)
        , Info(descr.Info)
    {
    }

    TPolynomDescrStatic(const TFormula fml,
            const TFormulaSSE fmlSSE,
            const SRelevanceFormula* descr,
            const char* name)
        : Fml(fml)
        , FmlSSE(fmlSSE)
        , Descr(descr)
        , Name(name)
    {
    }

    /// Calculate relevance with fastest possible function.
    float Calc(const float* factors) const;

    /// Calculate relevance for list of docs with fastest possible function.
    void MultiCalc(const float* const * factors, float *res, const size_t size) const;

    /// Calculate relevance for list of docs with fastest possible function.
    void MultiCalc(const TVector< TVector<float> > &factors, TVector<float> res) const;
};

/// Registry for static compiled polynoms.
class TRelevanceFormulaRegistry {
private:
    typedef THashMap<const char*, const TPolynomDescrStatic*> TStorage;
    TStorage Storage;

public:
    void Add(const TPolynomDescrStatic* fml);
    const TPolynomDescrStatic* Find(const char* name) const;
    void GetAllNames(TVector<TString>* result) const;
};

/// Get static polynoms registry singleton.
TRelevanceFormulaRegistry* RelevanceFormulaRegistry();

/*! Calculate coefficient in polynom for factorId.
 *
 * Put factors values (except factorId) in formula and
 * calculate total coefficient for factorId.
 */
double RelevFmlCalcFactor(const SRelevanceFormula &fml, const float *factors, int factorId);

