package ru.yandex.ci.engine.tasks.tracker;

import java.util.UUID;

import ci.tracker.create_issue.CreateIssueOuterClass;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;

@ExecutorInfo(
        title = "Create ticket in Yandex Tracker",
        description = "Создание релизного тикета в Трекере"
)
public class CreateIssueTestJob extends CreateIssueJob {

    public static final UUID ID = UUID.fromString("bc7c4e54-e395-42c0-b8c5-3f760a01ed9b");

    public CreateIssueTestJob(
            TrackerSessionSource trackerSessionSource,
            TrackerIssuesLinker trackerIssuesLinker,
            AbcService abcService
    ) {
        super(trackerSessionSource, trackerIssuesLinker, abcService);
    }

    @Override
    public void execute(JobContext context) {
        produceResources(
                context,
                context.resources().consume(CreateIssueOuterClass.Config.class),
                toProtoIssue("CI-123", true)
        );
    }

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

}
