package ru.yandex.ci.storage.core.logbroker.badge_events;

import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

public interface BadgeEventsSender {
    void sendEvent(CiBadgeEvents.Event event);

    void sendEvent(CheckEntity.Id checkId, CiBadgeEvents.Event event);
}
