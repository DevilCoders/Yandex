package ru.yandex.ci.engine.tasks.tracker;

import java.util.UUID;

import ci.tracker.create_issue.CreateIssueOuterClass;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;

// TODO: Temporary, until supporting Java tasklets in Sandbox
@RequiredArgsConstructor
@Slf4j
@ExecutorInfo(
        title = "Update ticket in Yandex Tracker",
        description = "Обновление релизного тикета в Трекере"
)
@Consume(name = "issue", proto = CreateIssueOuterClass.Issue.class)
@Consume(name = "config", proto = CreateIssueOuterClass.Config.class)
@Consume(name = "update_template", proto = CreateIssueOuterClass.UpdateTemplate.class)
public class UpdateIssueJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("9521283f-656a-4edc-8a6d-0e3704707ed4");

    private final TrackerSessionSource trackerSessionSource;
    private final TrackerIssuesLinker trackerIssuesLinker;

    @Override
    public void execute(JobContext context) throws Exception {
        var issue = context.resources().consume(CreateIssueOuterClass.Issue.class);
        var config = context.resources().consume(CreateIssueOuterClass.Config.class);
        var updateTemplate = context.resources().consume(CreateIssueOuterClass.UpdateTemplate.class);

        log.info("Updating issue using config: {}", issue);
        log.info("Using config: {}", config);
        log.info("Using update template: {}", updateTemplate);

        var flowLaunchContext = context.createFlowLaunchContext();
        var session = trackerSessionSource.getSession(config.getSecret(), flowLaunchContext.getYavTokenUid());

        var trackerIssue = session.issues().get(issue.getIssue());
        log.info("Loaded issue: {}", trackerIssue);

        var linker = trackerIssuesLinker.getIssuesLinker(config, flowLaunchContext);

        var updater = new LinkedIssuesUpdater(linker, session);
        updater.updateIssue(updateTemplate, trackerIssue);
        updater.updateAndTransitLinkedIssues(updateTemplate);

        CreateIssueJob.sendIssueBadge(trackerSessionSource, context, issue);
    }


    @Override
    public UUID getSourceCodeId() {
        return ID;
    }
}
