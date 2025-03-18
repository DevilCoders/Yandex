package ru.yandex.ci.storage.core.logbroker.badge_events;

import java.util.Map;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.Common.StorageAttribute;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.spring.BadgeEventsConfig;
import ru.yandex.ci.storage.core.spring.bazinga.SingleThreadBazingaServiceConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

@ContextConfiguration(classes = {
        BadgeEventsConfig.class,
        SingleThreadBazingaServiceConfig.class,
})
class BadgeEventsProducerTest extends StorageYdbTestBase {

    @Autowired
    BadgeEventsProducer badgeEventsProducer;
    @SpyBean
    BazingaTaskManager bazingaTaskManager;

    private final CheckEntity checkWithDisabledNotifications = sampleCheck.toBuilder()
            .type(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
            .attributes(Map.of(StorageAttribute.SA_NOTIFICATIONS_DISABLED, "true"))
            .build();

    @Test
    void onCheckCreated() {
        badgeEventsProducer.onCheckCreated(sampleCheck);
        verify(bazingaTaskManager, atLeastOnce()).schedule(any(BadgeEventSendTask.class));
    }


    @Test
    void onCheckCreated_whenNotificationsDisabled() {
        badgeEventsProducer.onCheckCreated(checkWithDisabledNotifications);
        verify(bazingaTaskManager, never()).schedule(any(BadgeEventSendTask.class));
    }

    @Test
    void onCheckFinished() {
        var check = sampleCheck.withStatus(CheckStatus.COMPLETED_SUCCESS);
        badgeEventsProducer.onCheckFinished(check);
        verify(bazingaTaskManager, atLeastOnce()).schedule(any(BadgeEventSendTask.class));
    }

    @Test
    void onCheckFinished_whenNotificationsDisabled() {
        var check = checkWithDisabledNotifications.withStatus(CheckStatus.COMPLETED_SUCCESS);
        badgeEventsProducer.onCheckFinished(check);
        verify(bazingaTaskManager, never()).schedule(any(BadgeEventSendTask.class));
    }

    @Test
    void onIterationTypeFinished() {
        var iteration = sampleIteration.withStatus(CheckStatus.COMPLETED_SUCCESS);
        badgeEventsProducer.onIterationTypeFinished(sampleCheck, iteration);
        verify(bazingaTaskManager, atLeastOnce()).schedule(any(BadgeEventSendTask.class));
    }

    @Test
    void onIterationTypeFinished_whenNotificationsDisabled() {
        var iteration = sampleIteration.withStatus(CheckStatus.COMPLETED_SUCCESS);
        badgeEventsProducer.onIterationTypeFinished(checkWithDisabledNotifications, iteration);
        verify(bazingaTaskManager, never()).schedule(any(BadgeEventSendTask.class));
    }

}
