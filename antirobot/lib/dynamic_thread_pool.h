#pragma once

#include "stats_output.h"
#include "stats_writer.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/tls.h>
#include <util/thread/factory.h>

#include <functional>


/*
 * Идея заимстована из QNX thread_pool_*
 *
 * Олег Цилюрик, Егор Горошко
 * QNX/UNIX: анатомия параллелизма
 * Издательство Символ Плюс, 2006
 * стр. 222-232
 * http://www.symbol.ru/alphabet/357604.html
 * http://lib.yandex-team.ru/books/2312/
 * http://flibusta.net/b/313854/read#t101
 */

#ifdef INFINITE
#   undef INFINITE
#endif

class TDynamicThreadPool: public IThreadFactory {
public:
    struct TParams {
        static const size_t INFINITE;
        static size_t ParseValue(const TString& valueStr);

        size_t MinFree;
        size_t MaxFree;
        size_t MaxTotal;
        size_t IncrementFree; // must be >= 1
    };

    class TThread {
    public:
        static TThread* Current() {
            return CurrentPtr;
        }

        void AddDestructor(std::function<void()> dtor) {
            Destructors.push_back(std::move(dtor));
        }

        template <typename T>
        void DeleteOnExit(T* object) {
            AddDestructor([object] () { delete object; });
        }

    protected:
        static thread_local TThread* CurrentPtr;
        TVector<std::function<void()>> Destructors;
    };

public:
    TDynamicThreadPool(const TParams& params);
    ~TDynamicThreadPool() override;

    void Stop();

    void PrintStatistics(NAntiRobot::TStatsWriter& out);
    void PrintStatistics(TStatsOutput& out);
private:
    IThread* DoCreate() override;
private:
    class TImpl;
    THolder <TImpl> Impl;
};
