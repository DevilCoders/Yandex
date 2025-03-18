package ru.yandex.ci.storage.tests;

import java.util.Deque;
import java.util.concurrent.ConcurrentLinkedDeque;

import javax.annotation.Nullable;

import lombok.Getter;

import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsSender;

public class BadgeEventsSenderTestImpl implements BadgeEventsSender {
    @Getter
    private final Deque<CiBadgeEvents.Event> events = new ConcurrentLinkedDeque<>();

    @Override
    public void sendEvent(CiBadgeEvents.Event event) {
        events.add(event);
    }

    @Override
    public void sendEvent(CheckEntity.Id checkId, CiBadgeEvents.Event event) {
        events.add(event);
    }

    @Nullable
    public CiBadgeEvents.Event nextEvent() {
        return events.pollFirst();
    }

    public BadgeEventsSenderTestImpl skip() {
        events.pollFirst();
        return this;
    }

    public boolean noMoreEvents() {
        return events.isEmpty();
    }

    public void clear() {
        events.clear();
    }
}
