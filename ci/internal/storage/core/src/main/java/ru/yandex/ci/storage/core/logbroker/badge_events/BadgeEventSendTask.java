package ru.yandex.ci.storage.core.logbroker.badge_events;

import java.time.Duration;

import com.google.protobuf.InvalidProtocolBufferException;
import lombok.Value;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;
import ru.yandex.misc.codec.Hex;

public class BadgeEventSendTask extends AbstractOnetimeTask<BadgeEventSendTask.Params> {
    private BadgeEventsSender badgeEventsSender;

    public BadgeEventSendTask(BadgeEventsSender badgeEventsSender) {
        super(Params.class);
        this.badgeEventsSender = badgeEventsSender;
    }

    public BadgeEventSendTask(CheckEntity.Id checkId, CiBadgeEvents.Event event) {
        super(Params.of(checkId, event));
    }

    @Override
    protected void execute(Params params, ExecutionContext context) throws Exception {
        badgeEventsSender.sendEvent(params.getCheckId(), params.getEvent());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        long checkId;

        String data;

        public static Params of(CheckEntity.Id checkId, CiBadgeEvents.Event event) {
            return new Params(checkId.getId(), Hex.encode(event.toByteArray()));
        }

        public CheckEntity.Id getCheckId() {
            return CheckEntity.Id.of(checkId);
        }

        public CiBadgeEvents.Event getEvent() {
            try {
                return CiBadgeEvents.Event.parseFrom(Hex.decode(data));
            } catch (InvalidProtocolBufferException e) {
                throw new RuntimeException("failed to decode message from task params", e);
            }
        }
    }

}
