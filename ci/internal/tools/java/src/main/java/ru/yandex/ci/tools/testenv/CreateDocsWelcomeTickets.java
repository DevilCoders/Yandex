package ru.yandex.ci.tools.testenv;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.beust.jcommander.Parameter;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.Getter;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.util.StreamUtils;

import ru.yandex.ci.client.tracker.TrackerClient;
import ru.yandex.ci.engine.spring.clients.TrackerClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.startrek.client.model.CommentCreate;
import ru.yandex.startrek.client.model.IssueCreate;
import ru.yandex.startrek.client.model.IssueUpdate;

import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.toList;

@Slf4j
@Configuration
@Import(TrackerClientConfig.class)
public class CreateDocsWelcomeTickets extends AbstractSpringBasedApp {

    private static final String CIWELCOME = "CIWELCOME";

    @Autowired
    private TrackerClient trackerClient;

    @Value("${ci.startrek.oauth}")
    // https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b
    private String trackerToken;

    @Parameter(names = "--do")
    private boolean doRun;

    @Override
    protected void run() throws Exception {
        var jobs = loadJobs();
        log.info("Load {} jobs", jobs.size());
        jobs = jobs.stream()
                .filter(j -> !j.getPaths().isEmpty())
                .toList();
        log.info("Jobs after filtering {}", jobs.size());
        var byOwners = groupByOwners(jobs);
        log.info("Made {} groups from {} jobs", byOwners.size(), jobs.size());

        for (var entry : byOwners.entrySet()) {
            var owners = entry.getKey();
            var jobsWithSameOwners = entry.getValue();
            createTicket(owners, jobsWithSameOwners);
        }
    }

    private void createTicket(Owners owners, List<Job> jobs) {
        var session = trackerClient.getSession(trackerToken);

        var docsTag = "docs:" + jobs.stream().map(Job::getName)
                .sorted()
                .collect(Collectors.joining("_"));

        var filter = "Queue: %s Tags: \"%s\"".formatted(CIWELCOME, docsTag);
        var foundTickets = session.issues().find(filter).toList();

        if (foundTickets.isEmpty()) {
            var issueCreate = IssueCreate.builder()
                    .queue(CIWELCOME)
                    .followers("gluk")
                    .tags("docs", docsTag)
                    .summary(createSummary(jobs))
                    .description(createIssueDescription(jobs))
                    .build();

            var summon = owners.getOwners();
            var commentCreate = CommentCreate.builder()
                    .summonees(summon.toArray(String[]::new))
                    .build();

            log.info("Creating issue ({}) {} and comment {} (summon {})...", docsTag, issueCreate, commentCreate,
                    summon);
            if (doRun) {
                var createdIssue = session.issues().create(issueCreate);
                var createdComment = session.comments().create(createdIssue, commentCreate);
                log.info("Created issue {} and comment {}", createdIssue, createdComment);
            }
        } else {
            Preconditions.checkState(foundTickets.size() == 1, "found more that one ticket with tag %s", docsTag);
            var issue = foundTickets.get(0);
            var issueUpdate = IssueUpdate
                    .description(createIssueDescription(jobs))
                    .summary(createSummary(jobs))
                    .build();

            log.info("Updating issue {}: {}", issue.getKey(), issueUpdate);
            if (doRun) {
                var updated = issue.update(issueUpdate
                );
                log.info("Issue {} updated", updated);
            }
        }
    }

    private String createSummary(List<Job> jobs) {
        return "Переезд документации %s"
                .formatted(jobs.stream().map(Job::getName).collect(Collectors.joining(" и ")));
    }

    private String createIssueDescription(List<Job> jobs) {
        var jobSettingsTemplate = """
                <[
                Вашу джобу в ТЕ **%s** следует удалить из файла [DeployDocs.py](https://a.yandex-team.ru/arc_vcs/testenv/jobs/docs/DeployDocs.py).
                Путь до проекта (path/to/your/project): `%s`
                Куда положить файл `a.yaml` в аркадии: `%s`
                ]>
                """;

        var jobsSettings = jobs.stream()
                .map(job -> jobSettingsTemplate.formatted(
                        job.getName(),
                        String.join(";", job.getTargets()),
                        job.getYamlPath()
                        )
                )
                .collect(Collectors.joining("\n\n"));

        var descriptionTemplate = readString("/deploy-docs/description.md");
        return descriptionTemplate.formatted(jobs.size(), jobsSettings);
    }

    private Map<Owners, List<Job>> groupByOwners(List<Job> jobs) {
        return jobs.stream()
                .collect(groupingBy(Job::getOwners, toList()));
    }

    private List<Job> loadJobs() {
        try {
            var deployDocsJson = readString("/deploy-docs/jobs.json");
            var mapper = new ObjectMapper();
            return mapper.readValue(deployDocsJson, new ListTypeReference());
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    private String readString(String resource) {
        try (var stream = CreateDocsWelcomeTickets.class.getResourceAsStream(resource)) {
            return StreamUtils.copyToString(stream, StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    @lombok.Value
    private static class Job {
        private static final List<String> NOT_EXIST_USER = List.of("elenbaskova", "abc:mediaanalyst");

        String name;
        List<String> observedPaths;
        List<String> owners;
        @Getter(AccessLevel.NONE)
        List<String> targetsToBuild;

        public Owners getOwners() {
            return Owners.of(owners.stream()
                    .filter(o -> !NOT_EXIST_USER.contains(o))
                    .toList());
        }

        public List<String> getPaths() {
            return Stream.concat(targetsToBuild.stream(), observedPaths.stream())
                    .filter(this::notBuildPlatform)
                    .distinct()
                    .toList();
        }

        public List<String> getTargets() {
            if (targetsToBuild.isEmpty()) {
                return getPaths();
            }
            return targetsToBuild;
        }

        private boolean notBuildPlatform(String path) {
            return !"build/platform/yfm".equals(path);
        }

        public String getYamlPath() {
            if (getPaths().size() == 1) {
                return getPaths().get(0);
            }
            var observed = observedPaths.stream()
                    .filter(this::notBuildPlatform)
                    .toList();
            if (observed.size() == 1) {
                return observed.get(0);
            }
            throw new RuntimeException("cannot get yaml path for " + this);
        }
    }

    @lombok.Value(staticConstructor = "of")
    private static class Owners {
        List<String> owners;
    }


    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

    private static class ListTypeReference extends TypeReference<List<Job>> {
    }

}
