package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.engine.spring.jobs.TrackerConfig;
import ru.yandex.ci.engine.tasks.tracker.CreateIssueJob;
import ru.yandex.ci.engine.tasks.tracker.TrackerIssuesLinker;
import ru.yandex.ci.engine.tasks.tracker.TrackerMigrationTickerParserJob;
import ru.yandex.ci.engine.tasks.tracker.TrackerSessionSource;
import ru.yandex.ci.engine.tasks.tracker.TransitIssueJob;
import ru.yandex.ci.engine.tasks.tracker.UpdateIssueJob;

@Configuration
@Import(TrackerConfig.class)
public class TrackerJobs {

    @Bean
    public CreateIssueJob createIssueJob(
            TrackerSessionSource trackerSessionSource,
            TrackerIssuesLinker trackerIssuesLinker,
            AbcService abcService
    ) {
        return new CreateIssueJob(trackerSessionSource, trackerIssuesLinker, abcService);
    }

    @Bean
    public TransitIssueJob transitIssueJob(
            TrackerSessionSource trackerSessionSource,
            TrackerIssuesLinker trackerIssuesLinker
    ) {
        return new TransitIssueJob(trackerSessionSource, trackerIssuesLinker);
    }

    @Bean
    public UpdateIssueJob updateIssueJob(
            TrackerSessionSource trackerSessionSource,
            TrackerIssuesLinker trackerIssuesLinker
    ) {
        return new UpdateIssueJob(trackerSessionSource, trackerIssuesLinker);
    }

    @Bean
    public TrackerMigrationTickerParserJob trackerMigrationTickerParserJob(TrackerSessionSource trackerSessionSource) {
        return new TrackerMigrationTickerParserJob(trackerSessionSource);
    }
}
