package ru.yandex.ci.storage.core.logbroker.badge_events;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

@Slf4j
public class BadgeEventsSenderEmptyImpl implements BadgeEventsSender {
    @Override
    public void sendEvent(CiBadgeEvents.Event event) {
        log.info("Sending event");
    }

    @Override
    public void sendEvent(CheckEntity.Id checkId, CiBadgeEvents.Event event) {
        log.info("Sending event for {}", checkId);
    }
}
