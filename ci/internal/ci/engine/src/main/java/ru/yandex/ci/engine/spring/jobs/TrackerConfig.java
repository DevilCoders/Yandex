package ru.yandex.ci.engine.spring.jobs;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.tracker.TrackerClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.engine.spring.SecurityServiceConfig;
import ru.yandex.ci.engine.spring.clients.TrackerClientConfig;
import ru.yandex.ci.engine.tasks.tracker.TrackerIssuesLinker;
import ru.yandex.ci.engine.tasks.tracker.TrackerSessionSource;
import ru.yandex.ci.engine.tasks.tracker.TrackerTicketCollector;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.utils.UrlService;

// Internal configs, will move to Java Tasklets
@Configuration
@Import({
        SecurityServiceConfig.class,
        ConfigurationServiceConfig.class,
        TrackerClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class TrackerConfig {

    @Bean
    public TrackerSessionSource trackerSessionSource(
            TrackerClient trackerClient,
            SecurityAccessService securityAccessService,
            @Value("${ci.trackerSessionSource.url}") String url
    ) {
        return new TrackerSessionSource(trackerClient, securityAccessService, url);
    }

    @Bean
    public TrackerIssuesLinker trackerIssuesLinker(
            CiDb db,
            UrlService urlService,
            CommitFetchService commitFetchService) {
        return new TrackerIssuesLinker(db, urlService, commitFetchService);
    }

    @Bean
    public TrackerTicketCollector trackerTicketCollector(TrackerSessionSource trackerSessionSource) {
        return new TrackerTicketCollector(trackerSessionSource);
    }
}
