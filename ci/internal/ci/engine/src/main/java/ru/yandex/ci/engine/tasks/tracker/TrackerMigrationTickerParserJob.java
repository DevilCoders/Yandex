package ru.yandex.ci.engine.tasks.tracker;

import java.util.UUID;

import ci.tracker.create_issue.CreateIssueOuterClass;
import ci.tracker.migration.Migration;
import ci.tracker.migration.Migration.MigrationConfig.RepositoryType;
import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import yav_service.YavOuterClass;

import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

// TODO: Temporary, until supporting Java tasklets in Sandbox
@RequiredArgsConstructor
@Slf4j
@ExecutorInfo(
        title = "Parse migration ticket in Yandex Tracker",
        description = "Разбор тикета для миграции в Трекере"
)
@Consume(name = "issue", proto = CreateIssueOuterClass.Issue.class)
@Consume(name = "config", proto = CreateIssueOuterClass.Config.class)
@Produces(single = {
        CreateIssueOuterClass.Issue.class,
        CreateIssueOuterClass.Config.class,
        Migration.MigrationConfig.class
})
public class TrackerMigrationTickerParserJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("e615bb8b-a984-4dfe-a7a9-047653d0a254");

    private final TrackerSessionSource trackerSessionSource;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var resources = context.resources();
        var issue = resources.consume(CreateIssueOuterClass.Issue.class);
        var config = resources.consume(CreateIssueOuterClass.Config.class);

        var flowLaunchContext = context.createFlowLaunchContext();
        var migration = parseIssue(config.getSecret(), flowLaunchContext.getYavTokenUid(), issue);

        resources.produce(Resource.of(issue, "issue"));
        resources.produce(Resource.of(config, "config"));
        resources.produce(Resource.of(migration, "migration"));
    }

    public Migration.MigrationConfig parseIssue(
            YavOuterClass.YavSecretSpec spec,
            YavToken.Id tokenId,
            CreateIssueOuterClass.Issue issue
    ) {
        log.info("Loading issue configuration: {}", issue);

        var session = trackerSessionSource.getSession(spec, tokenId);

        var issueObject = session.issues().get(issue.getIssue());
        log.info("Loaded issue: {}", issueObject);

        var description = issueObject.getDescription().get();
        var migration = parseMigrationConfig(description);

        log.info("Parsed migration config: {}", migration);
        return migration;
    }

    /*
    Type: git
    URL: https://github.yandex-team.ru/orc/orc-api
    Arcadia path: /contest/services/orc
    Branch: dev
    */
    private Migration.MigrationConfig parseMigrationConfig(String issueDescription) {
        var builder = Migration.MigrationConfig.newBuilder();
        for (var line : StringUtils.split(issueDescription, '\n')) {
            var parts = line.split(":", 2);
            if (parts.length != 2) {
                continue;
            }

            var prefix = parts[0].trim().toLowerCase();
            var value = StringUtils.strip(parts[1].trim(), "%`").trim();
            switch (prefix) {
                case "type" -> builder.setType(RepositoryType.valueOf(value.toLowerCase()));
                case "url" -> builder.setUrl(value);
                case "arcadia path" -> builder.setArcadiaPath(value);
                case "branch" -> builder.setGitBranch(value);
                default -> {
                }
            }
        }
        Preconditions.checkState(builder.getType() != RepositoryType.unknown, "Unable to fetch type from ticket");
        Preconditions.checkState(!builder.getUrl().isEmpty(), "Unable to fetch URL from ticket");
        Preconditions.checkState(!builder.getArcadiaPath().isEmpty(), "Unable to fetch Arcadia path from ticket");
        return builder.build();
    }
}
