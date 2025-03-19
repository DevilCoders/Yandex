#pragma once

#include "sequential.h"

#include <kernel/common_server/abstract/sessions.h>

#include <util/generic/map.h>

template <class TEvent>
class TSessionsBuilder {
private:
    using TEventsSessionPtr = typename ::IEventsSession<TEvent>::TPtr;
    using ISessionSelector = ::ISessionSelector<TEvent>;

private:
    TRWMutex Mutex;
    typename ISessionSelector::TPtr Selector;
    TMap<TString, TEventsSessionPtr> Sessions;
    TMap<TString, TEventsSessionPtr> TagSessions;
    TMap<TString, TDeque<TEventsSessionPtr>> UserSessions;
    TMap<TString, TDeque<TEventsSessionPtr>> ObjectSessions;

public:
    using TPtr = TAtomicSharedPtr<TSessionsBuilder<TEvent>>;

public:
    void CleanExpiredEvents(const TVector<TAtomicSharedPtr<TEvent>>& events) {
        if (events.empty()) {
            return;
        }
        TSet<TString> userIds;
        TSet<TString> objectIds;
        TSet<TString> sessionIds;
        {
            TReadGuard wg(Mutex);
            TMap<TString, ui64> objectsCheck;
            for (auto&& i : events) {
                auto it = objectsCheck.find(i->GetObjectId());
                if (it == objectsCheck.end()) {
                    objectsCheck.emplace(i->GetObjectId(), i->GetHistoryEventId());
                } else if (it->second < i->GetHistoryEventId()) {
                    it->second = i->GetHistoryEventId();
                }
            }
            for (auto&& i : objectsCheck) {
                auto it = ObjectSessions.find(i.first);
                if (it != ObjectSessions.end()) {
                    auto itScan = it->second.begin();
                    while (itScan != it->second.end() && (*itScan)->GetClosed() && (*itScan)->GetLastEventId() < i.second) {
                        objectIds.emplace((*itScan)->GetObjectId());
                        userIds.emplace((*itScan)->GetUserId());
                        sessionIds.emplace((*itScan)->GetSessionId());
                        (*itScan)->SetDeprecated(true);
                        ++itScan;
                    }
                }
            }
        }
        {
            TWriteGuard wg(Mutex);
            for (auto&& i : userIds) {
                auto it = UserSessions.find(i);
                if (it == UserSessions.end()) {
                    continue;
                }
                while (it->second.size() && it->second.front()->GetDeprecated()) {
                    it->second.pop_front();
                };
            }
            for (auto&& i : objectIds) {
                auto it = ObjectSessions.find(i);
                if (it == ObjectSessions.end()) {
                    continue;
                }
                while (it->second.size() && it->second.front()->GetDeprecated()) {
                    it->second.pop_front();
                };
            }
            for (auto&& i : sessionIds) {
                Sessions.erase(i);
            }
        }
    };

    bool IsEventForStoreSession(const TEvent& ev) const {
        if (!!Selector) {
            return Selector->Accept(ev) != NEventsSession::IgnoreExternal;
        } else {
            return false;
        }
    }

    bool IsEventForCloseSession(const TEvent& ev) const {
        if (!!Selector) {
            return Selector->Accept(ev) == NEventsSession::End;
        } else {
            return false;
        }
    }

    void FillAdditionalEventsConditions(TMap<TString, TSet<TString>>& result, const TDeque<TAtomicSharedPtr<TEvent>>& events, const ui64 historyEventIdMaxWithFinishInstant) const {
        if (!!Selector) {
            Selector->FillAdditionalEventsConditions(result, events, historyEventIdMaxWithFinishInstant);
        }
    }

    TEventsSessionPtr GetLastObjectSession(const TString& objectId) const {
        TReadGuard rg(Mutex);
        auto it = ObjectSessions.find(objectId);
        if (it == ObjectSessions.end() || it->second.empty()) {
            return nullptr;
        }
        return it->second.back();
    }

    TEventsSessionPtr GetLastObjectUserSession(const TString& objectId, const TString& userId) const {
        TReadGuard rg(Mutex);
        auto it = UserSessions.find(userId);
        if (it == UserSessions.end() || it->second.empty()) {
            return nullptr;
        }

        for (auto&& i = it->second.rbegin(); i != it->second.rend(); ++i) {
            if ((*i)->GetObjectId() == objectId) {
                return *i;
            }
        }

        return nullptr;
    }

    void SignalRefresh(const TInstant refreshInstant) {
        TReadGuard rg(Mutex);
        for (auto&& userSession : ObjectSessions) {
            if (!userSession.second.back()->GetClosed()) {
                userSession.second.back()->SignalRefresh(refreshInstant);
            }
        }
    }

    TEventsSessionPtr GetSession(const TString& sessionId) const {
        TReadGuard rg(Mutex);
        auto it = Sessions.find(sessionId);
        if (it == Sessions.end()) {
            return nullptr;
        }
        return it->second;
    }

    TVector<TEventsSessionPtr> GetUserSessions(const TString& userId) const {
        TReadGuard rg(Mutex);
        auto it = UserSessions.find(userId);
        TVector<TEventsSessionPtr> result;
        if (it == UserSessions.end()) {
            return result;
        }
        result.reserve(it->second.size());
        result.insert(result.end(), it->second.begin(), it->second.end());
        return result;
    }

    template <class TActor>
    bool ApplyOperator(const TString& objectId, const TActor& actor) const {
        TReadGuard rg(Mutex);
        auto it = ObjectSessions.find(objectId);
        if (it == ObjectSessions.end()) {
            return false;
        }
        actor(it->second);
        return true;
    }

    TVector<TEventsSessionPtr> GetObjectSessions(const TString& objectId) const {
        TReadGuard rg(Mutex);
        auto it = ObjectSessions.find(objectId);
        TVector<TEventsSessionPtr> result;
        if (it == ObjectSessions.end()) {
            return result;
        }
        result.reserve(it->second.size());
        result.insert(result.end(), it->second.begin(), it->second.end());
        return result;
    }

    void FillResultSession(const TInstant since, const TInstant until, const TString& id, const TDeque<TEventsSessionPtr>& sessions, TMap<TString, TVector<TEventsSessionPtr>>& result) {
        auto itResult = result.end();
        for (auto it = sessions.rbegin(); it != sessions.rend(); ++it) {
            if ((*it)->GetLastTS() < since) {
                break;
            }
            if ((*it)->GetStartTS() > until) {
                continue;
            }
            if (itResult == result.end()) {
                itResult = result.emplace(id, TVector<TEventsSessionPtr>()).first;
            }
            itResult->second.emplace_back(*it);
        }
    }

    TMap<TString, TVector<TEventsSessionPtr>> GetSessionsActualImpl(const TInstant since, const TInstant until, const TVector<TString>& ids, const TMap<TString, TDeque<TEventsSessionPtr>>& sessions) {
        TMap<TString, TVector<TEventsSessionPtr>> result;
        for (auto&& id : ids) {
            result.emplace(id, TVector<TEventsSessionPtr>());
            auto itSessions = sessions.find(id);
            if (itSessions == sessions.end()) {
                continue;
            }

            FillResultSession(since, until, itSessions->first, itSessions->second, result);

        }
        return result;
    }

    TMap<TString, TVector<TEventsSessionPtr>> GetSessionsActualByObjects(const TInstant since, const TInstant until, const TVector<TString>& objectIds) {
        TReadGuard rg(Mutex);
        return GetSessionsActualImpl(since, until, objectIds, ObjectSessions);
    }

    TVector<TEventsSessionPtr> GetSessionsActualSinceId(const ui64 eventId) const {
        TReadGuard rg(Mutex);
        TVector<TEventsSessionPtr> result;
        for (auto&& session : Sessions) {
            if (session.second->GetLastEventId() > eventId) {
                result.emplace_back(session.second);
            }
        }
        return result;
    }

    TMap<TString, TVector<TEventsSessionPtr>> GetSessionsActual(const TInstant since, const TInstant until) {
        TMap<TString, TVector<TEventsSessionPtr>> result;
        TReadGuard rg(Mutex);
        for (auto&& session : ObjectSessions) {
            for (auto it = session.second.rbegin(); it != session.second.rend(); ++it) {
                if ((*it)->GetLastTS() < since) {
                    break;
                }
                if ((*it)->GetStartTS() >= until) {
                    continue;
                }
                result[(*it)->GetUserId()].emplace_back(*it);
            }
        }
        return result;
    }

    TVector<TEventsSessionPtr> GetSessionsActual() {
        TReadGuard rg(Mutex);
        TVector<TEventsSessionPtr> result;
        for (auto&& session : ObjectSessions) {
            if (session.second.size() && !session.second.back()->GetClosed()) {
                result.emplace_back(session.second.back());
            }
        }
        return result;
    }

    TMap<TString, TEventsSessionPtr> GetSessionsActualByObjects() {
        TReadGuard rg(Mutex);
        TMap<TString, TEventsSessionPtr> result;
        for (auto&& session : ObjectSessions) {
            if (session.second.size() && !session.second.back()->GetClosed()) {
                result.emplace(session.first, session.second.back());
            }
        }
        return result;
    }

    TMap<TString, TEventsSessionPtr> GetSessionsActualByObjects(const TSet<TString>* objectIds) {
        if (!objectIds) {
            return GetSessionsActualByObjects();
        }
        TReadGuard rg(Mutex);
        TMap<TString, TEventsSessionPtr> result;
        if (objectIds->size() < ObjectSessions.size() / 10) {
            for (auto&& i : *objectIds) {
                auto it = ObjectSessions.find(i);
                if (it != ObjectSessions.end() && it->second.size() && !it->second.back()->GetClosed()) {
                    result.emplace(i, it->second.back());
                }
            }
        } else {
            auto it = ObjectSessions.begin();
            for (auto&& i : *objectIds) {
                if (Advance(it, ObjectSessions.end(), i) && it->second.size() && !it->second.back()->GetClosed()) {
                    result.emplace(i, it->second.back());
                }
            }
        }
        return result;
    }

    void AddEvent(TAtomicSharedPtr<TEvent> e) {
        if (!e || !TEvent::GetInstanceId(*e)) {
            return;
        }
        const NEventsSession::EEventCategory cat = Selector->Accept(*e);
        if (cat == NEventsSession::EEventCategory::IgnoreExternal || cat == NEventsSession::EEventCategory::IgnoreInternal) {
            return;
        }
        TWriteGuard wg(Mutex);
        auto it = TagSessions.find(TEvent::GetInstanceId(*e));
        if (cat == NEventsSession::Switch && it != TagSessions.end() && !it->second->GetClosed()) {
            it->second->AddEvent(e, NEventsSession::End);
        }
        if (cat == NEventsSession::Begin || cat == NEventsSession::Switch) {
            if (it != TagSessions.end()) {
                it->second->CheckClosed();
            }
            auto session = Selector->BuildSession();
            session->AddEvent(e, NEventsSession::Begin);
            Sessions[session->GetSessionId()] = session;
            TagSessions[TEvent::GetInstanceId(*e)] = session;
            UserSessions[e->GetHistoryUserId()].emplace_back(session);
            ObjectSessions[TEvent::GetObjectId(*e)].emplace_back(session);
        } else if (it != TagSessions.end()) {
            it->second->AddEvent(e, cat);
        }
    }

    TSessionsBuilder(typename ISessionSelector::TPtr selector, const TDeque<TAtomicSharedPtr<TEvent>>& events)
        : Selector(selector)
    {
        for (auto&& e : events) {
            AddEvent(e);
        }
    }
};

template <class TEvent>
class TSequentialTableWithSessions: public TIndexedSequentialTableImpl<TEvent> {
private:
    using ISessionSelector = ::ISessionSelector<TEvent>;
    using TSessionsBuilder = ::TSessionsBuilder<TEvent>;
    using TBase = TIndexedSequentialTableImpl<TEvent>;

private:
    TRWMutex MutexSessions;
    TMap<TString, typename TSessionsBuilder::TPtr> Sessions;

protected:

    bool IsEventForStoreSession(const TEvent& ev) const {
        for (auto&& i : Sessions) {
            if (i.second->IsEventForStoreSession(ev)) {
                return true;
            }
        }
        return false;
    }

    bool IsEventForCloseSession(const TEvent& ev) const {
        for (auto&& i : Sessions) {
            if (i.second->IsEventForCloseSession(ev)) {
                return true;
            }
        }
        return false;
    }

    virtual void OnFinishCacheModification(const typename TBase::TParsedObjects& records) const override {
        TReadGuard rgSessions(MutexSessions);
        for (auto&& sessionBuilder : Sessions) {
            sessionBuilder.second->SignalRefresh(records.GetModelingInstantNow());
        }
    }

    virtual void AddEventDeep(TAtomicSharedPtr<TEvent> eventObj) const override final {
        for (auto&& sessionsBuilder : Sessions) {
            sessionsBuilder.second->AddEvent(eventObj);
        }
    }

    virtual void FillAdditionalEventsConditions(TMap<TString, TSet<TString>>& result, const ui64 historyEventIdMaxWithFinishInstant) const override {
        for (auto&& i : Sessions) {
            i.second->FillAdditionalEventsConditions(result, TBase::Events, historyEventIdMaxWithFinishInstant);
        }
    }

    virtual TVector<TAtomicSharedPtr<TEvent>> CleanExpiredEvents(const TInstant deadline) const {
        auto events = TBase::CleanExpiredEvents(deadline);
        TReadGuard rgSessions(MutexSessions);
        for (auto&& sessionBuilder : Sessions) {
            sessionBuilder.second->CleanExpiredEvents(events);
        }
        return events;
    }
public:
    using TBase::TBase;

    typename TSessionsBuilder::TPtr GetSessionsBuilderSafe(const TString& name, const TInstant reqActuality) const {
        if (!TBase::Update(reqActuality)) {
            return nullptr;
        }
        TReadGuard wg(MutexSessions);
        auto it = Sessions.find(name);
        if (it == Sessions.end()) {
            return nullptr;
        }
        return it->second;
    }

    typename TSessionsBuilder::TPtr GetSessionsBuilderSafe(const TString& name, const TDuration maxAge) const {
        return GetSessionsBuilderSafe(name, Now() - maxAge);
    }

    typename TSessionsBuilder::TPtr GetSessionsBuilder(const TString& name, const TDuration maxAge) const {
        return GetSessionsBuilder(name, Now() - maxAge);
    }

    typename TSessionsBuilder::TPtr GetSessionsBuilder(const TString& name, const TInstant reqActuality) const {
        if (!TBase::Update(reqActuality)) {
            WARNING_LOG << "Cannot refresh sessions builder data for " << name << Endl;
        }
        TReadGuard wg(MutexSessions);
        auto it = Sessions.find(name);
        if (it == Sessions.end()) {
            return nullptr;
        }
        return it->second;
    }

    void RegisterSessionBuilder(typename ISessionSelector::TPtr sessionSelector) {
        TWriteGuard wg(MutexSessions);
        Sessions.emplace(sessionSelector->GetName(), MakeAtomicShared<TSessionsBuilder>(sessionSelector, TBase::Events));
    }
};
