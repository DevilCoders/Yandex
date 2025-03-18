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
@Slf4j
@RequiredArgsConstructor
@ExecutorInfo(
        title = "Moved ticket in Yandex Tracker to next stage",
        description = "Продвижение релизного тикета в Трекере по статусам"
)
@Consume(name = "issue", proto = CreateIssueOuterClass.Issue.class)
@Consume(name = "config", proto = CreateIssueOuterClass.Config.class)
@Consume(name = "transition", proto = CreateIssueOuterClass.Transition.class)
@Consume(name = "update_template", proto = CreateIssueOuterClass.UpdateTemplate.class)
public class TransitIssueJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("17223f13-c75d-4418-99ec-bd3d8ad3e0f1");

    private final TrackerSessionSource trackerSessionSource;
    private final TrackerIssuesLinker trackerIssuesLinker;

    @Override
    public void execute(JobContext context) throws Exception {
        var issue = context.resources().consume(CreateIssueOuterClass.Issue.class);
        var config = context.resources().consume(CreateIssueOuterClass.Config.class);
        var transition = context.resources().consume(CreateIssueOuterClass.Transition.class);
        var updateTemplate = context.resources().consume(CreateIssueOuterClass.UpdateTemplate.class);

        log.info("Moving issue using config: {}", issue);
        log.info("Using config: {}", config);
        log.info("Using transition: {}", transition);
        log.info("Using update template: {}", updateTemplate);

        var flowLaunchContext = context.createFlowLaunchContext();
        var session = trackerSessionSource.getSession(config.getSecret(), flowLaunchContext.getYavTokenUid());

        var trackerIssue = session.issues().get(issue.getIssue());
        log.info("Loaded issue: {}", trackerIssue);

        var linker = trackerIssuesLinker.getIssuesLinker(config, flowLaunchContext);

        var updater = new LinkedIssuesUpdater(linker, session);
        updater.transitIssue(transition, trackerIssue);
        updater.updateIssue(updateTemplate, trackerIssue);
        updater.updateAndTransitLinkedIssues(updateTemplate);

        CreateIssueJob.sendIssueBadge(trackerSessionSource, context, issue);
    }

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }
}
