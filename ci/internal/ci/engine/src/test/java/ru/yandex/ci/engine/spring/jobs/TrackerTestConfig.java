package ru.yandex.ci.engine.spring.jobs;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.spring.AbcStubConfig;
import ru.yandex.ci.engine.tasks.tracker.CreateIssueTestJob;
import ru.yandex.ci.engine.tasks.tracker.TrackerIssuesLinker;
import ru.yandex.ci.engine.tasks.tracker.TrackerSessionSource;

@Configuration
@Import(AbcStubConfig.class)
public class TrackerTestConfig {

    @Bean
    public TrackerSessionSource trackerSessionSource() {
        return Mockito.mock(TrackerSessionSource.class);
    }

    @Bean
    public TrackerIssuesLinker trackerIssuesLinker() {
        return Mockito.mock(TrackerIssuesLinker.class);
    }

    @Bean
    public CreateIssueTestJob createIssueTestJob(
            TrackerSessionSource trackerSessionSource,
            TrackerIssuesLinker trackerIssuesLinker,
            AbcService abcService
    ) {
        return new CreateIssueTestJob(trackerSessionSource, trackerIssuesLinker, abcService);
    }

}
