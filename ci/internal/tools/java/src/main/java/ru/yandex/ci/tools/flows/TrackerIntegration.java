package ru.yandex.ci.tools.flows;

import java.nio.file.Path;
import java.time.Instant;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.function.Consumer;

import javax.annotation.PostConstruct;

import ci.tracker.create_issue.CreateIssueOuterClass;
import ci.tracker.create_issue.CreateIssueOuterClass.Rules.OnDuplicate;
import com.google.common.base.Preconditions;
import com.google.protobuf.GeneratedMessageV3;
import lombok.extern.slf4j.Slf4j;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.stubbing.Answer;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import yav_service.YavOuterClass;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.spring.jobs.TrackerConfig;
import ru.yandex.ci.engine.tasks.tracker.CreateIssueJob;
import ru.yandex.ci.engine.tasks.tracker.TrackerIssuesLinker;
import ru.yandex.ci.engine.tasks.tracker.TrackerSessionSource;
import ru.yandex.ci.engine.tasks.tracker.TransitIssueJob;
import ru.yandex.ci.engine.tasks.tracker.UpdateIssueJob;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.context.JobProgressContext;
import ru.yandex.ci.flow.engine.definition.context.JobResourcesContext;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.job.GetCommitsRequest;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.startrek.client.model.SearchRequest;
import ru.yandex.startrek.client.model.ServiceRef;

@Slf4j
@Import(TrackerConfig.class)
@Configuration
public class TrackerIntegration extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Autowired
    TrackerSessionSource tracker;

    @Autowired
    TrackerIssuesLinker trackerIssuesLinker;

    @Autowired
    AbcService abcService;

    private String dir;
    private String id;

    private Version createVersion;
    private Version updateVersion;
    private Version update2Version;

    private CreateIssueOuterClass.Config config;

    @PostConstruct
    void init() {
        dir = "ci/demo-project";
        id = "demo-sawmill-release-branches";
        createVersion = Version.fromAsString("8");
        updateVersion = Version.fromAsString("8.1");
        update2Version = Version.fromAsString("8.2");

        config = CreateIssueOuterClass.Config.newBuilder()
                .setSecret(getYavSecretSpec())
                .setLink(CreateIssueOuterClass.LinkingRules.newBuilder()
                        .setType(GetCommitsRequest.Type.FROM_PREVIOUS_STABLE)
                        .addQueues("CI")
                        .addQueues("CIDEMO"))
                .build();
    }

    void checkFields() {
        var context = getLaunchContext(createVersion);
        var session = tracker.getSession(config.getSecret(), context.getYavTokenUid());

        var issue = session.issues().get("CIDEMO-115");
        List<ServiceRef> abcService = issue.get("abcService");
        for (var abc : abcService) {
            log.info("abc: {}", abc.getId());
        }
    }

    @Override
    protected void run() throws Exception {
        cleanupIssues();
    }

    void testIssues() {
        var session = tracker.getSession(config.getSecret(), getYavTokenId());

        var issue = session.issues().get("GRUT-360");
        log.info("Issue: {}", issue);
    }

    void cleanupIssues() {
        var version = this.createVersion;
        var context = getLaunchContext(version);
        var session = tracker.getSession(config.getSecret(), context.getYavTokenUid());

        var template = getTemplate(version);

        var request = SearchRequest.builder()
                .filter("queue", template.getQueue())
                .build();
        var issues = session.issues().find(request);
        var exclude = Set.of("CIDEMO-139", "CIDEMO-118", "CIDEMO-119");
        issues.stream().forEach(issue -> {
            var key = issue.getKey();
            if (exclude.contains(key)) {
                return; // ---
            }
            var links = issue.getLinks();
            if (links.isEmpty()) {
                return; // ---
            }
            log.info("Unlinking issues: {}", key);
            for (var link : links) {
                session.links().delete(issue, link);
            }
        });
    }

    @SuppressWarnings("BusyWait")
    void testTracker() throws Exception {
        var issue = normalFlow();
        log.info("Registered new ticket: {}", issue.getIssue());

        while (!Thread.interrupted()) {
            Thread.sleep(10000);
            try {
                failOnDuplicateFlow();
                throw new IllegalStateException("Must FAIL on duplicate");
            } catch (RuntimeException e) {
                var prefix = "Found duplicate issue for version %s: ".formatted(getVersion(update2Version));
                var ticket = issue.getIssue();
                var expectMessage = prefix + ticket;
                var actualMessage = e.getMessage();

                log.info("Waiting for {}", expectMessage);
                log.info("Actual message {}", actualMessage);
                if (actualMessage.startsWith(prefix)) {
                    if (actualMessage.equals(expectMessage)) {
                        break;
                    } else {
                        continue;
                    }
                }
                throw e;
            }
        }

        Thread.sleep(5000);
        var updateIssue = updateFlow();
        Preconditions.checkState(Objects.equals(issue, updateIssue),
                "Expect both issues are same: %s and %s", issue, updateIssue);
    }

    private CreateIssueOuterClass.Issue normalFlow() throws Exception {
        transitIssue(updateVersion,
                CreateIssueOuterClass.Issue.newBuilder()
                        .setIssue("CIDEMO-118")
                        .build(),
                CreateIssueOuterClass.Transition.newBuilder()
                        .setStatus("Открыт")
                        .setIgnoreNoTransition(true)
                        .build(),
                CreateIssueOuterClass.UpdateTemplate.getDefaultInstance());

        var issue = createIssue(createVersion,
                builder -> {
                },
                builder -> {
                });
        transitIssue(createVersion, issue,
                CreateIssueOuterClass.Transition.newBuilder()
                        .setStatus("inProgress") // transition id
                        .build(),
                CreateIssueOuterClass.UpdateTemplate.getDefaultInstance()
        );
        updateIssue(updateVersion, issue);

        // Close issue
        transitIssue(updateVersion, issue,
                CreateIssueOuterClass.Transition.newBuilder()
                        .setStatus("closed") // status key
                        .setResolution("fixed") // resolution key
                        .build(),
                CreateIssueOuterClass.UpdateTemplate.getDefaultInstance());

        transitIssue(updateVersion, issue,
                // Issue already closed but this will be ignored
                CreateIssueOuterClass.Transition.newBuilder()
                        .setStatus("closed") // status key
                        .setResolution("fixed") // resolution key
                        .build(),
                CreateIssueOuterClass.UpdateTemplate.newBuilder()
                        .setComment("Issues is closed")
                        .putLinkedQueues("CIDEMO", CreateIssueOuterClass.LinkedIssueUpdate.newBuilder()
                                .setComment("Тест переходов")
                                .setTransition(CreateIssueOuterClass.Transition.newBuilder()
                                        .setStatus("В работе")
                                        .build())
                                .build())
                        .build()
        );
        return issue;
    }

    private CreateIssueOuterClass.Issue failOnDuplicateFlow() throws Exception {
        return createIssue(update2Version,
                builder -> builder.setOnDuplicate(OnDuplicate.FAIL),
                builder -> {
                });
    }

    private CreateIssueOuterClass.Issue updateFlow() throws Exception {
        return createIssue(update2Version,
                builder -> builder.setOnDuplicate(OnDuplicate.UPDATE),
                builder -> builder.putLinkedQueues("CIDEMO", CreateIssueOuterClass.LinkedIssueUpdate.newBuilder()
                        .setComment("New release ticket created for ((%s))".formatted(getUrl(update2Version)))
                        .build()));
    }

    private CreateIssueOuterClass.Issue createIssue(
            Version releaseVersion,
            Consumer<CreateIssueOuterClass.Rules.Builder> rulesUpdate,
            Consumer<CreateIssueOuterClass.UpdateTemplate.Builder> commentsUpdate) throws Exception {
        var flowLaunchContext = getLaunchContext(releaseVersion);
        var version = flowLaunchContext.getLaunchInfo().getVersion();
        var template = getTemplate(version);

        var commentTemplate = getCommentTemplate(version).toBuilder();
        commentsUpdate.accept(commentTemplate);

        var rules = CreateIssueOuterClass.Rules.newBuilder()
                .setOnDuplicate(OnDuplicate.NEW);
        rulesUpdate.accept(rules);

        var job = new CreateIssueJob(tracker, trackerIssuesLinker, abcService);
        var jobContext = getJobContext(flowLaunchContext, config, rules.build(), template, commentTemplate.build());
        job.execute(jobContext);

        var captor = ArgumentCaptor.forClass(Resource.class);
        Mockito.verify(jobContext.resources(), Mockito.times(2)).produce(captor.capture());

        var values = captor.getAllValues();
        var msg = values.stream()
                .filter(value -> "issue".equals(value.getParentField()))
                .findFirst()
                .map(Resource::getData)
                .orElseThrow();

        return CreateIssueOuterClass.Issue.newBuilder()
                .setIssue(msg.get("issue").getAsString())
                .build();
    }

    private void transitIssue(Version releaseVersion,
                              CreateIssueOuterClass.Issue issue,
                              CreateIssueOuterClass.Transition transition,
                              CreateIssueOuterClass.UpdateTemplate updateTemplate) throws Exception {
        var flowLaunchContext = getLaunchContext(releaseVersion);
        var jobContext = getJobContext(flowLaunchContext, issue, config, transition, updateTemplate);
        var job = new TransitIssueJob(tracker, trackerIssuesLinker);
        job.execute(jobContext);
    }

    private void updateIssue(Version releaseVersion, CreateIssueOuterClass.Issue issue) throws Exception {
        var flowLaunchContext = getLaunchContext(releaseVersion);
        var version = flowLaunchContext.getLaunchInfo().getVersion();

        var updateTemplate = getCommentTemplate(version);
        var jobContext = getJobContext(flowLaunchContext, issue, config, updateTemplate);
        var job = new UpdateIssueJob(tracker, trackerIssuesLinker);
        job.execute(jobContext);
    }

    static YavOuterClass.YavSecretSpec getYavSecretSpec() {
        return YavOuterClass.YavSecretSpec.newBuilder()
                .setKey("startrek.token")
                .build();
    }

    static YavToken.Id getYavTokenId() {
        return YavToken.Id.of("tid-01fs24sdvrzbnq5t4kvna7gcrk");
    }

    private FlowLaunchContext getLaunchContext(Version version) {
        var processId = CiProcessId.ofRelease(Path.of(dir + "/a.yaml"), id);
        var launch = db.currentOrReadOnly(() ->
                db.launches().findLaunchesByVersion(processId, version))
                .orElseThrow();
        return FlowLaunchContext.builder()
                .sandboxOwner("CI")
                .yavTokenUid(getYavTokenId())
                .launchInfo(LaunchInfo.of(launch.getVersion()))
                .selectedBranch(ArcBranch.trunk())
                .projectId("ci")
                .launchId(launch.getLaunchId())
                .targetRevision(launch.getVcsInfo().getRevision())
                .flowStarted(Objects.requireNonNullElse(launch.getStarted(), Instant.now()))
                .jobStarted(Objects.requireNonNullElse(launch.getStarted(), Instant.now())) // Actually, does not matter
                .build();
    }

    private JobContext getJobContext(FlowLaunchContext flowLaunchContext, GeneratedMessageV3... messages) {
        var jobProgressContext = Mockito.mock(JobProgressContext.class);
        var resourcesContext = Mockito.mock(JobResourcesContext.class);
        var jobContext = Mockito.mock(JobContext.class);
        Mockito.when(jobContext.progress()).thenReturn(jobProgressContext);
        Mockito.when(jobContext.resources()).thenReturn(resourcesContext);
        Mockito.when(jobContext.createFlowLaunchContext()).thenReturn(flowLaunchContext);

        for (var message : messages) {
            Mockito.when(resourcesContext.consume(message.getClass()))
                    .thenAnswer((Answer<GeneratedMessageV3>) invocation -> message);
        }
        return jobContext;
    }

    private String getTitle(Version version) {
        return "Demo sawmill release (branches) #%s".formatted(version.asString());
    }

    private String getUrl(Version version) {
        return "https://a.yandex-team.ru/projects/ci/ci/releases/flow?dir=%s&id=%s&version=%s"
                .formatted(dir.replace("/", "%2F"), id, version.asString());
    }

    private String getVersion(Version version) {
        return "CIDEMO-" + version.getMajor();
    }

    private CreateIssueOuterClass.Template getTemplate(Version version) {
        var template = CreateIssueOuterClass.Template.newBuilder()
                .setFixVersion(getVersion(version))
                .setQueue("CIDEMO")
                .setType("Релиз")
                .setSummary("Релиз %s".formatted(getTitle(version)))
                .addComponents("компонент-2")
                .addComponents("component-1")
                .addAbcServices("cidemo")
                .addAbcServices("33049")
                .setDescription("""
                        == %s
                        * Версия: %s
                        * Ссылка в CI: ((%s))
                        * Ревизия: {{revision}}
                        == Тикеты
                        {{issues}}
                        ==+Коммиты
                        {{commits}}
                          """.formatted(getTitle(version), version.asString(), getUrl(version)));
        template.addChecklistBuilder()
                .setText("Collect underpants")
                .setChecked(true);
        template.addChecklistBuilder()
                .setText("?");
        template.addChecklistBuilder()
                .setText("Profit");
        return template.build();
    }

    private CreateIssueOuterClass.UpdateTemplate getCommentTemplate(Version version) {
        return CreateIssueOuterClass.UpdateTemplate.newBuilder()
                .setComment("""
                        Новые тикеты и коммиты в релизе ((%s %s)).
                        == Тикеты
                        {{issues}}
                        ==+Коммиты
                        {{commits}}""".formatted(getUrl(version), getTitle(version)))
                .build();
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

}
