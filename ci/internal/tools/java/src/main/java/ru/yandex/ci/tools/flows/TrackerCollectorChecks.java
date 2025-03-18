package ru.yandex.ci.tools.flows;

import java.nio.file.Path;
import java.util.List;
import java.util.Set;

import ci.tracker.create_issue.CreateIssueOuterClass;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.TrackerWatchConfig;
import ru.yandex.ci.engine.discovery.tracker_watcher.TrackerWatcher;
import ru.yandex.ci.engine.spring.jobs.TrackerConfig;
import ru.yandex.ci.engine.spring.tasks.EngineTasksConfig;
import ru.yandex.ci.engine.tasks.tracker.TrackerMigrationTickerParserJob;
import ru.yandex.ci.engine.tasks.tracker.TrackerSessionSource;
import ru.yandex.ci.engine.tasks.tracker.TrackerTicketCollector;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import({TrackerConfig.class, EngineTasksConfig.class})
@Configuration
public class TrackerCollectorChecks extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Autowired
    TrackerTicketCollector collector;

    @Autowired
    TrackerSessionSource trackerSessionSource;

    @Autowired
    TrackerWatcher trackerWatcher;

    @Override
    protected void run() throws Exception {
        parseTickets();
    }

    void testTrackerWatcher() {
        var ciProcessId = CiProcessId.ofFlow(Path.of("devtools/migration/a.yaml"), "run-migration");

        var configState = db.currentOrReadOnly(() -> db.configStates().get(ciProcessId.getPath()));
        var trackerCfg = TrackerWatchConfig.builder()
                .queue("TOARCADIA")
                .issues(Set.of("TOARCADIA-1"))
                .status("readyForStart")
                .closeStatuses(List.of("open", "closed"))
                .flowVar("issue")
                .secret(TrackerWatchConfig.YavSecretSpec.builder()
                        .key("startrek.token")
                        .build())
                .build();
        trackerWatcher.processSingle(configState, ciProcessId, trackerCfg);
    }

    void parseTickets() {
        var spec = TrackerIntegration.getYavSecretSpec();
        var token = TrackerIntegration.getYavTokenId();

        var parser = new TrackerMigrationTickerParserJob(trackerSessionSource);
        var issues = collector.getIssues(spec, token, "TOARCADIA", "inProgress");
        for (var issue : issues) {
            log.info("\n{} (by {}, at {})\n{}",
                    issue.getKey(),
                    issue.getAssignee(),
                    issue.getStatusUpdated(),
                    parser.parseIssue(spec, token, issue(issue.getKey())));
        }
    }

    private static CreateIssueOuterClass.Issue issue(String issue) {
        return CreateIssueOuterClass.Issue.newBuilder().setIssue(issue).build();
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

}
