package ru.yandex.ci.storage.core.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.autoconfigure.condition.ConditionalOnMissingBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventSendTask;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsSender;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsSenderEmptyImpl;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsSenderImpl;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;


@Configuration
@Import({
        CommonConfig.class,
        LogbrokerConfig.class,
        BazingaCoreConfig.class
})
public class BadgeEventsConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    @ConditionalOnMissingBean
    public BadgeEventsSender localBadgeEventsSender() {
        return new BadgeEventsSenderEmptyImpl();
    }

    @Bean
    @Profile(CiProfile.STABLE_PROFILE)
    public BadgeEventsSender badgeEventsSender(
            LogbrokerWriterFactory badgeEventsLogbrokerWriterFactory,
            @Value("${storage.badgeEventsSender.numberOfPartitions}") int numberOfPartitions
    ) {
        return new BadgeEventsSenderImpl(numberOfPartitions, meterRegistry, badgeEventsLogbrokerWriterFactory);
    }

    @Bean
    @Profile(CiProfile.STABLE_PROFILE)
    public LogbrokerWriterFactory badgeEventsLogbrokerWriterFactory(
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider,
            @Value("${storage.badgeEventsSender.topic}") String topic
    ) {
        return new LogbrokerWriterFactoryImpl(
                topic,
                "badge_events",
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }

    @Bean
    public BadgeEventsProducer badgeEventsProducer(BazingaTaskManager bazingaTaskManager) {
        return new BadgeEventsProducer(bazingaTaskManager);
    }

    @Bean
    public BadgeEventSendTask badgeEventSendTask(BadgeEventsSender badgeEventsSender) {
        return new BadgeEventSendTask(badgeEventsSender);
    }
}
