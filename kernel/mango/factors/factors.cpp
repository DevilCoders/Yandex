#include "factors.h"
#include <util/ysaveload.h>
#include <util/generic/algorithm.h>
#include <util/stream/str.h>
#include <util/datetime/base.h>
#include <util/generic/ymath.h>
#include <util/string/cast.h>
#include <kernel/web_factors_info/factor_names.h>

#define REGISTER_FACTOR(factor) manager.Register(prefix + #factor, TGroupTraits<group>::Offset + factor);

namespace NMango
{
    static_assert(N_FACTOR_COUNT >= TGroupTraits<GROUP_LAST>::EndOffset, "");

    int TFactorsManager::TDescriptor::operator & (IBinSaver& f) {
        f.Add(2, &Id);
        f.Add(3, &Name);
        return 0;
    }

    int TFactors::operator & (IBinSaver& f) {
        TString buf;
        if (f.IsReading()) {
            f.Add(2, &buf);
            Load(buf);
        } else {
            Save(buf);
            f.Add(2, &buf);
        }
        return 0;
    }

    float& TFactors::operator[] (int id)
    {
        Y_ASSERT(id >= 0);
        if (id >= Values.ysize())
            Values.resize(id + 1, std::numeric_limits<float>::quiet_NaN());
        return Values[id];
    }

    float TFactors::operator[] (int id) const
    {
        if (id >= Values.ysize() || id < 0)
            return std::numeric_limits<float>::quiet_NaN();
        return Values[id];
    }

    TFactors::TFactors()
    {
        Values.assign(GetFactorsCount(), std::numeric_limits<float>::quiet_NaN());
    }

    TFactors::TFactors(const float *values)
    {
        Values.assign(values, values + GetFactorsCount());
    }

    size_t TFactors::GetFactorsCount()
    {
        return TGroupTraits<GROUP_LAST>::EndOffset + 1;
    }

    void TFactors::Clear()
    {
        Values.assign(Values.size(), std::numeric_limits<float>::quiet_NaN());
    }

    void TFactors::Swap(TFactors& other)
    {
        Values.swap(other.Values);
    }

    bool TFactors::IsDefined(int id) const
    {
        return !IsNan(Values[id]);
    }

    void TFactors::Save(IOutputStream& out) const
    {
        size_t cnt(0);
        for (size_t i = 0; i < Values.size(); ++i) {
            if (IsDefined(i)) {
                if (i > 255) {
                    ythrow yexception() << "TFactors::Save: id > 255, id = " << i;
                }
                ++cnt;
            }
        }
        if (cnt > 255) {
            ythrow yexception() << "TFactors::Save: factors count > 255, count = " << cnt;
        }
        ::Save(&out, (ui8)cnt);
        for (size_t i = 0; i < Values.size(); ++i) {
            if (IsDefined(i)) {
                ::Save(&out, (ui8)i);
                ::Save(&out, (float)Values[i]);
            }
        }
    }

    void TFactors::Load(IInputStream& inp)
    {
        Clear();
        ui8 cnt(0);
        ::Load(&inp, cnt);
        for (ui8 i = 0; i < cnt; ++i) {
            ui8 index;
            float value;
            ::Load(&inp, index);
            ::Load(&inp, value);
            (*this)[index] = value;
        }
    }

    void TFactors::Save(TString& out) const
    {
        TStringOutput fout(out);
        Save(fout);
    }

    void TFactors::Load(const TString& inp)
    {
        TStringInput finp(inp);
        Load(finp);
    }

    void TFactors::CopyToArray(float *dest)
    {
        Copy(Values.begin(), Values.end(), dest);
    }

    void TFactors::Print() const
    {
        for (size_t i = 0; i < Values.size(); ++i)
            if (IsDefined(i)) {
                Cerr << TFactorsManager::InstanceConst()->GetName(i, false) << "(" << i << ")" << '=' << Values[i] << Endl;
            }
    }

    TString TFactors::ToString() const
    {
        TString out("");
        for (size_t i = 0; i < Values.size(); ++i) {
            if (IsDefined(i))
                out += TFactorsManager::InstanceConst()->GetName(i, false) + "(" + ::ToString(i) + ")=" + ::ToString(Values[i]) + "\n";
        }
        return out;
    }

    void TFactors::ApplyAll(const TFactors &other)
    {
        for (int i = 0, max_i = Min(Size(), other.Size()); i < max_i; ++i) {
            if (other.IsDefined(i)) {
                Values[i] = other[i];
            }
        }
    }

    void TFactorsManager::Register(const TString &name, int id)
    {
        FactorMap[name] = id;
        NameById[id] = name;
    }

    int TFactorsManager::GetId(const TString &name, bool throwOnFail) const
    {
        THashMap<TString, int>::const_iterator it = FactorMap.find(name);
        if (it != FactorMap.end())
            return it->second;
        if (throwOnFail)
            ythrow yexception() << "Factor with given name=" << name << " does not exist";
        return -1;
    }

    const TString& TFactorsManager::GetName(int id, bool throwOnFail) const
    {
        const static TString empty("");
        THashMap<int, TString>::const_iterator it = NameById.find(id);
        if (it == NameById.end()) {
            if (!throwOnFail)
                return empty;
            ythrow yexception() << "Factor with given id=" << id << " does not exist";
        }
        return it->second;
    }

    void TFactors::ClearArray(float *factors) {
        memset(factors, 0, sizeof(float) * TGroupTraits<GROUP_LAST>::EndOffset);
    }

    template<TFactorGroups group>
    void RegisterFactorGroupQuote(TFactorsManager &manager, const TString& prefix)
    {
        REGISTER_FACTOR( LINKS_COUNT );
        REGISTER_FACTOR( RESOURCE_COUNT );
        REGISTER_FACTOR( MAX_AUTHORITY );
        REGISTER_FACTOR( AUTHOR_COUNT );
        REGISTER_FACTOR( LANGS_COUNT );
        REGISTER_FACTOR( DIVERSITY );
        REGISTER_FACTOR( AUTHOR_AUTHORITY_SUM );
        REGISTER_FACTOR( QUOTE_AUTHORITY_SUM );
        REGISTER_FACTOR( AUTHOR_AUTHORITY_Q50 );
        REGISTER_FACTOR( AUTHOR_AUTHORITY_Q75 );
        REGISTER_FACTOR( AUTHOR_AUTHORITY_Q90 );
        REGISTER_FACTOR( AUTHOR_AUTHORITY_AVG75 );
        REGISTER_FACTOR( SPAM_COUNT );

        REGISTER_FACTOR( SPAM_PROBABILITY );
        REGISTER_FACTOR( AVERAGE_QUOTE_AUTHORITY );
        REGISTER_FACTOR( AVERAGE_AUTHOR_AUTHORITY );
        REGISTER_FACTOR( UNIQUE_AUTHORS_RATIO );
        REGISTER_FACTOR( UNIQUE_AUTHORS_RATIO_BY_AUTHORITY );
    }

    template<TFactorGroups group>
    void RegisterFactorGroupStaticOther(TFactorsManager &manager, const TString& prefix)
    {
        REGISTER_FACTOR( IS_MORDA );
        REGISTER_FACTOR( CREATION_TIME );
        REGISTER_FACTOR( TWEET_COUNT );
        REGISTER_FACTOR( LIKE_AUTHORITY_SUM );
        REGISTER_FACTOR( LIKE_COUNT );
        REGISTER_FACTOR( AUTH_PREDICT_KOEF );
        REGISTER_FACTOR( AUTHORITY_STATRANK );
    }

    template<TFactorGroups group>
    void RegisterFactorGroupRelevance(TFactorsManager &manager, const TString& prefix)
    {
        REGISTER_FACTOR( COS_TR );
        REGISTER_FACTOR( MANGO_TR );
        REGISTER_FACTOR( PHRASE_TR );
        REGISTER_FACTOR( TITLE_TR );
    }

    template<TFactorGroups group>
    void RegisterFactorGroupDocRelevance(TFactorsManager &manager, const TString& prefix)
    {
        REGISTER_FACTOR( POS_RELEVANCE );
        REGISTER_FACTOR( POS_RELEVANCE_NORM_BY_DOC );
        REGISTER_FACTOR( POS_RELEVANCE_NORM_BY_QUERY );
        REGISTER_FACTOR( FLAT_RELEVANCE );
    }

    template<TFactorGroups group>
    void RegisterFactorGroupOther(TFactorsManager &manager, const TString& prefix)
    {
        REGISTER_FACTOR( TOPIC_SHARE );
        REGISTER_FACTOR( AUTHOR_TOPIC_SHARE );
        REGISTER_FACTOR( UNWEIGHTED_TOPIC_SHARE );
        REGISTER_FACTOR( UNWEIGHTED_AUTHOR_TOPIC_SHARE );
        REGISTER_FACTOR( DECAYED_AUTHOR_AUTHORITY_SUM );
        REGISTER_FACTOR( DECAYED_QUOTE_AUTHORITY_SUM );
        REGISTER_FACTOR( FORCED_LINKS_COUNT );
        REGISTER_FACTOR( MEGA_FORCED_LINKS_COUNT );
        REGISTER_FACTOR( TITLE_LINKS_COUNT );
        REGISTER_FACTOR( NORMALIZED_LINKS_COUNT );
    }

    template<TFactorGroups group>
    void RegisterFactorGroupFinal(TFactorsManager &manager, const TString& prefix)
    {
        REGISTER_FACTOR( SIMPLE_RELEVANCE_SUM );
        REGISTER_FACTOR( SIMPLE_RELEVANCE_AVG );
        REGISTER_FACTOR( SIMPLE_RELEVANCE_AT );
        REGISTER_FACTOR( SIMPLE_RELEVANCE_MNG_SUM );
        REGISTER_FACTOR( SIMPLE_RELEVANCE_MNG_AVG );
        REGISTER_FACTOR( SIMPLE_RELEVANCE_MNG_AT );
        REGISTER_FACTOR( SIMPLE_INTERESTINGNESS );
        REGISTER_FACTOR( SIMPLE_PESSIMISATION );
        REGISTER_FACTOR( SIMPLE_TRASHLESS );
        REGISTER_FACTOR( SIMPLE_STATIC_INTERESTINGNESS );
        REGISTER_FACTOR( SIMPLE_STATIC_TRASHLESS );
        REGISTER_FACTOR( SIMPLE_HOST_INTERESTINGNESS );
        REGISTER_FACTOR( SIMPLE_HOST_TRASHLESS );
        REGISTER_FACTOR( SIMPLE_FORMULA_1 );
    }

#define REGISTER_FACTOR_WINDOW(I) \
    RegisterFactorGroupQuote    <Y_CAT(GROUP_W##I, _QUOTE_REL)>    (*this, TString("MANGO_") + WindowDescriptors[I].Marker + "_REL_"); \
    RegisterFactorGroupRelevance<Y_CAT(GROUP_W##I, _RELEVANCE_SUM)>(*this, TString("MANGO_") + WindowDescriptors[I].Marker + "_REL_SUM_"); \
    RegisterFactorGroupRelevance<Y_CAT(GROUP_W##I, _RELEVANCE_AVG)>(*this, TString("MANGO_") + WindowDescriptors[I].Marker + "_REL_AVG_"); \
    RegisterFactorGroupRelevance<Y_CAT(GROUP_W##I, _RELEVANCE_AT)> (*this, TString("MANGO_") + WindowDescriptors[I].Marker + "_REL_AT_"); \
    RegisterFactorGroupOther    <Y_CAT(GROUP_W##I, _OTHER)>        (*this, TString("MANGO_") + WindowDescriptors[I].Marker + "_REL_"); \
    RegisterFactorGroupFinal    <Y_CAT(GROUP_W##I, _FINAL)>        (*this, TString("MANGO_") + WindowDescriptors[I].Marker + "_");


    void TFactorsManager::Init()
    {
        RegisterFactorGroupQuote      <GROUP_QUOTE_STATIC>    (*this, "MANGO_STATIC_");
        RegisterFactorGroupQuote      <GROUP_QUOTE_HOST>      (*this, "MANGO_HOST_");
        RegisterFactorGroupStaticOther<GROUP_OTHER_STATIC>    (*this, "MANGO_STATIC_");

        REPEAT_WINDOWS(REGISTER_FACTOR_WINDOW)

        RegisterFactorGroupDocRelevance<GROUP_DOC_RELEVANCE>             (*this, "MANGO_DOC_RELEVANCE_");
    }

    void TFactorsManager::List(TVector<TFactorsManager::TDescriptor>& factors) const
    {
        factors.clear();
        for (THashMap<int, TString>::const_iterator it = NameById.begin(); it != NameById.end(); ++it)
            factors.push_back(TFactorsManager::TDescriptor(it->first, it->second));
    }

    // ---- TDecayCalculator -----

    TDecayCalculator::TDecayCalculator(time_t start /*= 0*/, time_t finish /*= 0*/, float fading /*= 0.1*/)
    {
        if (!start) {
            start = DEFAULT_TIME_FROM;
        }
        if (!finish) {
            finish = Now().TimeT(); // = now
        }
        FinishTime = finish;
        K = log(fading) / (finish - start);
        LastTime = 0;
    }

    float TDecayCalculator::CalcDecay(time_t time) const
    {
        if (time != LastTime) {
            LastResult = exp((FinishTime - time) * K);
            LastTime = time;
        }
        return LastResult;
    }

}
