#pragma once

#include "common.h"

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/instant_model.h>

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/global.h>

#include <util/datetime/base.h>
#include <util/generic/cast.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/system/mutex.h>
#include "frontend.h"

class IDriveTagsMeta;

class IFrontend;

namespace NCS {
    class IObjectSnapshot;
}

class TSessionCorruptedGlobalMessage: public IMessage {
private:
    RTLINE_ACCEPTOR(TSessionCorruptedGlobalMessage, NeedAlert, bool, true)
public:
};

class ISessionReportCustomization {
public:
    RTLINE_ACCEPTOR(ISessionReportCustomization, NeedMeta, bool, true);

public:
    using TPtr = TAtomicSharedPtr<ISessionReportCustomization>;

public:
    virtual ~ISessionReportCustomization() = default;
};

class ISession {
public:
    using TPtr = TAtomicSharedPtr<ISession>;
    enum class ECurrentState {
        Undefined,
        Started,
        Closed,
        Corrupted
    };
public:
    RTLINE_READONLY_PROTECT_ACCEPTOR(CurrentState, ECurrentState, ECurrentState::Undefined);
    RTLINE_READONLY_PROTECT_ACCEPTOR(StartTS, TInstant, TInstant::Zero());
    RTLINE_READONLY_PROTECT_ACCEPTOR(LastTS, TInstant, TInstant::Zero());
    RTLINE_READONLY_PROTECT_ACCEPTOR(LastEventId, ui64, 0);
    RTLINE_READONLY_PROTECT_ACCEPTOR(ObjectId, TString, "");
    RTLINE_READONLY_PROTECT_ACCEPTOR(UserId, TString, "");
    RTLINE_READONLY_MUTABLE_ACCEPTOR(Compiled, bool, false);
    RTLINE_READONLY_MUTABLE_ACCEPTOR(InstanceId, TString, "");
    RTLINE_ACCEPTOR(ISession, Deprecated, bool, false);

protected:
    TMutex Mutex;

protected:
    virtual bool DoCompile() const = 0;
    virtual NJson::TJsonValue DoGetReport(const IFrontend* server, ISessionReportCustomization::TPtr customization) const = 0;
    NJson::TJsonValue GetReportMeta() const;

public:
    ISession() = default;
    virtual ~ISession() = default;

    bool GetClosed() const {
        return CurrentState == ECurrentState::Closed;
    }

    bool GetStarted() const {
        return CurrentState == ECurrentState::Started || CurrentState == ECurrentState::Closed;
    }

    virtual const TString& GetSessionId() const = 0;

    bool OccuredInInterval(const TInstant since, const TInstant until) const;

    void SignalRefresh(const TInstant refreshIntant) {
        TGuard<TMutex> g(Mutex);
        if (!GetClosed() && refreshIntant > LastTS) {
            LastTS = refreshIntant;
            Compiled = false;
        }
    }

    NJson::TJsonValue GetReport(const IFrontend* server, ISessionReportCustomization::TPtr customization) const {
        TGuard<TMutex> g(Mutex);
        Compile();
        NJson::TJsonValue result;
        if (customization->GetNeedMeta()) {
            result.InsertValue("meta", GetReportMeta());
        }
        result.InsertValue("session", DoGetReport(server, customization));
        return result;
    }

    void SetInstanceId(const TString& value) {
        InstanceId = value;
    }

    void ResetCompile() const {
        Compiled = false;
    }

    virtual bool Compile() const final {
        TGuard<TMutex> g(Mutex);
        if (Compiled && GetClosed()) {
            return true;
        }
        if (CurrentState == ECurrentState::Corrupted) {
            return false;
        }
        Compiled = DoCompile();
        return Compiled;
    }
};

namespace NEventsSession {
    const i64 MaxEventId = Max<i64>();

    enum EEventCategory {
        Begin = 1,
        End = 1 << 1,
        Internal = 1 << 2,
        IgnoreExternal = 1 << 3,
        IgnoreInternal = 1 << 4,
        Switch = 1 << 5,
    };
}

template <class TEvent>
class IEventsSession: public ISession {
public:
    enum class EEvent {
        Tag,
        CurrentFinish
    };

    class TTimeEvent {
    public:
        RTLINE_ACCEPTOR(TTimeEvent, TimeEvent, EEvent, EEvent::Tag);
        RTLINE_ACCEPTOR(TTimeEvent, EventId, i64, -1);
        RTLINE_ACCEPTOR(TTimeEvent, EventInstant, TInstant, TInstant::Zero());
        RTLINE_ACCEPTOR(TTimeEvent, EventIndex, ui32, Max<ui32>());

    public:
        bool operator<(const TTimeEvent& item) const {
            if (EventInstant == item.EventInstant) {
                return EventId < item.EventId;
            } else {
                return EventInstant < item.EventInstant;
            }
        }
    };

    class ICompilation {
    public:
        virtual ~ICompilation() = default;
        virtual bool Fill(const TVector<TTimeEvent>& timeline, const TVector<TAtomicSharedPtr<TEvent>>& events) = 0;
        virtual NJson::TJsonValue GetReport(const IFrontend* server, ISessionReportCustomization::TPtr customization) const = 0;
        virtual const TString& GetSessionId() const = 0;
    };

    using TPtr = TAtomicSharedPtr<IEventsSession<TEvent>>;

protected:
    virtual ICompilation* BuildDefaultCompilation() const {
        return nullptr;
    }

    virtual bool DoCompile() const override final {
        if (!DefaultCompilation) {
            DefaultCompilation.Reset(BuildDefaultCompilation());
            CHECK_WITH_LOG(!!DefaultCompilation);
        }
        return FillCompilation(*DefaultCompilation);
    }

    virtual NJson::TJsonValue DoGetReport(const IFrontend* server, ISessionReportCustomization::TPtr customization) const override final {
        NJson::TJsonValue result;
        CHECK_WITH_LOG(!!DefaultCompilation);
        result.InsertValue("specials", DefaultCompilation->GetReport(server, customization));
        return result;
    }

private:
    mutable THolder<ICompilation> DefaultCompilation;
    TVector<TAtomicSharedPtr<TEvent>> Events;
    TAtomicSharedPtr<TEvent> CurrentEvent;
    mutable TVector<TTimeEvent> Timeline;
    mutable ui32 PredTimelineSize = 0;
    void PrepareTimeline() const {
        if (PredTimelineSize == Timeline.size()) {
            return;
        }
        std::sort(Timeline.begin(), Timeline.end());
        PredTimelineSize = Timeline.size();
    }
protected:
    bool EventsEmpty() const {
        return Events.empty();
    }
public:

    TMaybe<TEvent> GetLastEvent() const {
        TGuard<TMutex> g(Mutex);
        if (CurrentEvent) {
            return *CurrentEvent;
        }
        return {};
    }

    TMaybe<TEvent> GetFirstEvent() const {
        TGuard<TMutex> g(Mutex);
        PrepareTimeline();
        if (Timeline.size() && Timeline.front().GetTimeEvent() == EEvent::Tag) {
            return *Events[Timeline.front().GetEventIndex()];
        }
        return {};
    }

    template <class T>
    TMaybe<T> DetachCompilationAs() const {
        TGuard<TMutex> g(Mutex);
        if (!Compile()) {
            WARNING_LOG << "Cannot compile session: " << GetSessionId() << Endl;
            return {};
        }
        const T* val = dynamic_cast<const T*>(DefaultCompilation.Get());
        if (val) {
            return *val;
        } else {
            return {};
        }
    }

    virtual const TString& GetSessionId() const override {
        TGuard<TMutex> g(Mutex);
        if (!DefaultCompilation) {
            if (!Compile()) {
                return Default<TString>();
            }
        }
        return DefaultCompilation->GetSessionId();
    }

    bool FillCompilation(ICompilation& compilation) const {
        TGuard<TMutex> g(Mutex);
        PrepareTimeline();
        TVector<TTimeEvent> timeline = Timeline;
        if (Timeline.size()) {
            TInstant max = Timeline[0].GetEventInstant();
            for (ui32 i = 0; i + 1 < Timeline.size(); ++i) {
                CHECK_WITH_LOG(Timeline[i].GetEventId() != Timeline[i + 1].GetEventId());
                CHECK_WITH_LOG(Timeline[i].GetEventInstant() <= Timeline[i + 1].GetEventInstant());
                max = Max(max, Timeline[i + 1].GetEventInstant());
            }
            if (!GetClosed()) {
                TTimeEvent eventPoint;
                eventPoint.SetTimeEvent(EEvent::CurrentFinish).SetEventInstant(Max(max, LastTS)).SetEventId(NEventsSession::MaxEventId);
                if (timeline.empty() || timeline.back().GetTimeEvent() != EEvent::CurrentFinish) {
                    timeline.emplace_back(eventPoint);
                }
            }
        }
        return compilation.Fill(timeline, Events);
    }

    class TDefaultItemReportConstructor {
    public:
        static void DoBuildReportItem(const TEvent* ev, NJson::TJsonValue& item) {
            ev->DoBuildReportItem(item);
        }
    };

    bool HasEvents(const TInstant since = TInstant::Zero(), const TInstant until = TInstant::Max()) const {
        TGuard<TMutex> g(Mutex);
        Compile();
        std::sort(Timeline.begin(), Timeline.end());
        for (auto&& i : Timeline) {
            if (i.GetEventInstant() >= since && i.GetEventInstant() < until) {
                return true;
            }
        }
        return false;
    }

    template <class TItemReportConstructor = TDefaultItemReportConstructor>
    NJson::TJsonValue GetEventsReport(const TInstant since = TInstant::Zero(), const TInstant until = TInstant::Max()) const {
        TGuard<TMutex> g(Mutex);
        NJson::TJsonValue result(NJson::JSON_MAP);
        result.InsertValue("timeline", GetTimelineReport<TItemReportConstructor>(since, until));
        result.InsertValue("meta", ISession::GetReportMeta());
        return result;
    }

    template <class TItemReportConstructor = TDefaultItemReportConstructor>
    NJson::TJsonValue GetTimelineReport(const TInstant since = TInstant::Zero(), const TInstant until = TInstant::Max()) const {
        TGuard<TMutex> g(Mutex);
        Compile();
        std::sort(Timeline.begin(), Timeline.end());
        NJson::TJsonValue jsonTimeline(NJson::JSON_ARRAY);
        for (auto&& i : Timeline) {
            if (i.GetEventInstant() >= since && i.GetEventInstant() < until) {
                jsonTimeline.AppendValue(Events[i.GetEventIndex()]->template BuildReportItemCustom<TItemReportConstructor>());
            }
        }
        return jsonTimeline;
    }

    void MarkCorrupted() {
        TSessionCorruptedGlobalMessage message;
        SendGlobalMessage(message);
        if (message.GetNeedAlert()) {
            ALERT_LOG << "Session corrupted: " << GetInstanceId() << "/" << UserId << "/" << ObjectId << "/" << LastEventId << "/" << Events.size() << Endl;
        } else {
            WARNING_LOG << "Session corrupted: " << GetInstanceId() << "/" << UserId << "/" << ObjectId << "/" << LastEventId << "/" << Events.size() << Endl;
        }
        CurrentState = ECurrentState::Corrupted;
    }

    virtual bool TestEvent(const TEvent& histEvent) const = 0;

    void InitializeSession(const TEvent& histEvent, const NEventsSession::EEventCategory cat) {
        StartTS = histEvent.GetHistoryInstant();
        LastTS = histEvent.GetHistoryInstant();
        LastEventId = histEvent.GetHistoryEventId();
        ObjectId = TEvent::GetObjectId(histEvent);
        UserId = histEvent.GetHistoryUserId();
        SetInstanceId(TEvent::GetInstanceId(histEvent));
        CurrentState = ISession::ECurrentState::Started;
        if (cat != NEventsSession::EEventCategory::Begin && cat != NEventsSession::EEventCategory::Switch) {
            MarkCorrupted();
        }
    }

    void InsertInternalEvent(TAtomicSharedPtr<TEvent> histEvent) {
        if (histEvent->GetHistoryInstant() < GetStartTS()) {
            StartTS = histEvent->GetHistoryInstant();
        }
        if (histEvent->GetHistoryEventId() > GetLastEventId()) {
            LastEventId = histEvent->GetHistoryEventId();
        }
        if (histEvent->GetHistoryInstant() > GetLastTS()) {
            LastTS = histEvent->GetHistoryInstant();
        }
        Events.emplace_back(histEvent);

        TTimeEvent eventPoint;
        eventPoint.SetEventIndex(Events.size() - 1).SetEventInstant(histEvent->GetHistoryInstant()).SetEventId(histEvent->GetHistoryEventId()).SetTimeEvent(EEvent::Tag);
        Timeline.emplace_back(eventPoint);
        if (!CurrentEvent || *CurrentEvent < *histEvent) {
            CurrentEvent = histEvent;
        }
        ResetCompile();
    }

    void CheckClosed() noexcept {
        if (CurrentState != ISession::ECurrentState::Closed) {
            MarkCorrupted();
        }
    }

    void AddEvent(TAtomicSharedPtr<TEvent> histEvent, const NEventsSession::EEventCategory cat) noexcept {
        TGuard<TMutex> g(Mutex);
        if (CurrentState == ECurrentState::Corrupted) {
            return;
        }
        Y_ASSERT(!CurrentEvent || (CurrentEvent->GetHistoryEventId() != histEvent->GetHistoryEventId()));
        if (CurrentState == ISession::ECurrentState::Undefined || CurrentState == ISession::ECurrentState::Started) {
            if (CurrentState == ISession::ECurrentState::Undefined) {
                InitializeSession(*histEvent, cat);
            } else {
                if (cat == NEventsSession::EEventCategory::Internal) {

                } else if (cat == NEventsSession::EEventCategory::End) {
                    CurrentState = ISession::ECurrentState::Closed;
                } else {
                    MarkCorrupted();
                    return;
                }
            }
            if (!TestEvent(*histEvent)) {
                MarkCorrupted();
                return;
            }
            InsertInternalEvent(histEvent);
        } else if (CurrentState != ISession::ECurrentState::Closed) {
            MarkCorrupted();
        }
    }
};

template <class TEvent>
class ISessionSelector {
public:
    using TPtr = TAtomicSharedPtr<ISessionSelector<TEvent>>;

public:
    virtual ~ISessionSelector() = default;

    virtual NEventsSession::EEventCategory Accept(const TEvent& e) const = 0;

    virtual void FillAdditionalEventsConditions(TMap<TString, TSet<TString>>& /*result*/, const TDeque<TAtomicSharedPtr<TEvent>>& /*events*/, const ui64 /*historyEventIdMaxWithFinishInstant*/) const {

    }
    virtual TAtomicSharedPtr<IEventsSession<TEvent>> BuildSession() const = 0;
    virtual TString GetName() const = 0;
};
