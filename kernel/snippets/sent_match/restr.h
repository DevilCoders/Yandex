#pragma once

#include <util/generic/vector.h>

namespace NSnippets
{
    class TConfig;
    class TSentsInfo;
    class TSentsMatchInfo;
    class IRestr
    {
    public:
        virtual bool operator()(int sentId) const = 0;
        virtual ~IRestr() {
        }
    };

    class TSkippedRestr : public IRestr
    {
    private:
        const TVector<bool> MRestr;

    public:
        TSkippedRestr(bool byLink, const TSentsInfo& sentsInfo, const TConfig& cfg);

        bool operator()(int i) const override
        {
            return MRestr[i];
        }
    };

    class TParaRestr : public IRestr
    {
    private:
        const TVector<bool> MRestr;

    public:
        TParaRestr(const TSentsInfo& sentsInfo);

        bool operator()(int i) const override
        {
            return MRestr[i];
        }
    };

    class TQualRestr : public IRestr
    {
    private:
        const TVector<bool> MRestr;

    public:
        TQualRestr(const TSentsMatchInfo& info);

        bool operator()(int i) const override
        {
            return MRestr[i];
        }
    };

    class TSegmentRestr : public IRestr
    {
    private:
        const TVector<bool> MRestr;

    public:
        TSegmentRestr(const TSentsMatchInfo& info);

        bool operator()(int i) const override
        {
            return MRestr[i];
        }
    };

    class TSimilarRestr : public IRestr
    {
    private:
        TVector<bool> MRestr;

    public:
        TSimilarRestr(const TSentsMatchInfo& info);

        bool operator()(int sentId) const override
        {
            return MRestr[sentId];
        }
    };

    class TRestr : public IRestr
    {
    private:
        TVector<IRestr*> vecRestrs;
    public:
        TRestr()
        {
        };
        void AddRestr(IRestr* restr)
        {
            vecRestrs.push_back(restr);
        }
        bool operator()(int sentId) const override
        {
            for(TVector<IRestr*>::const_iterator it = vecRestrs.begin(); it != vecRestrs.end(); ++it)
                if ((*it)->operator()(sentId))
                    return true;
            return false;
        }
    };

    class TNoRestr : public IRestr
    {
    public:
        bool operator()(int) const override
        {
            return false;
        }
    };
}
