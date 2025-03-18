package ru.yandex.ci.tools.testenv.migration;

import java.time.LocalDate;
import java.time.ZoneId;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Splitter;
import com.google.common.base.Strings;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;
import org.springframework.jdbc.core.JdbcTemplate;

import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.testenv.model.TestenvJob;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.startrek.client.Session;
import ru.yandex.startrek.client.model.Issue;
import ru.yandex.startrek.client.model.SearchRequest;

@SuppressWarnings("unused")
@Slf4j
public class TestenvMigrationService {
    private final JdbcTemplate jdbcTemplate;
    private final TestenvClient testenvClient;

    private final Session trackerSession;

    private static final String TAG_DB_PREFIX = "DB:";

    private final Map<String, RmMigrationStatus> teDb2RmMigrationStatuses;

    private final Map<String, Issue> teDb2CiWelcomeIssue;

    public TestenvMigrationService(JdbcTemplate jdbcTemplate, TestenvClient testenvClient, Session trackerSession) {
        this.jdbcTemplate = jdbcTemplate;
        this.testenvClient = testenvClient;
        this.trackerSession = trackerSession;
        teDb2RmMigrationStatuses = getTeDBToMigrationStatuses();
        teDb2CiWelcomeIssue = getTeDb2CiWelcomeIssues();
    }

    /**
     * Метод специально написан так, что надает при любой подозрительной проблеме с компонентом.
     * Таким компонентны надо просматривать руками.
     */
    public void stopInactiveRmComponents(List<String> rmComponents) {
        var rmComponentToProject = StreamEx.of(getActiveProjects())
                .filter(p -> p.getRmComponent() != null)
                .toMap(MigratingTestenvProject::getRmComponent, Function.identity());

        for (String rmComponent : rmComponents) {
            log.info("Processing RM component: {}", rmComponent);
            var project = rmComponentToProject.get(rmComponent);
            Preconditions.checkState(project != null);
            Preconditions.checkState(project.getRmTicket() != null);
            log.info(
                    "Stopping RM component {}, TM DB {}, Ticket {}",
                    rmComponent, project.getName(), project.getRmTicket()
            );
            stopRmComponent(project.getName(), project.getRmTicket());
            log.info("TE DB {} for RM component {} stopped.", project.getName(), rmComponent);
        }
    }

    public void stopRmComponent(String teDb, String rmTicket) {
        var issue = trackerSession.issues().get(rmTicket);
        Preconditions.checkState(isNoReaction(issue));

        Preconditions.checkState(issue.getQueue().getKey().equals("RMDEV"));

        var assignee = issue.getAssignee().orElseThrow();

        String teDbUrl = "https://testenv.yandex-team.ru/?screen=manage&database=" + teDb;

        testenvClient.stopProject(
                teDb,
                "Details: https://st.yandex-team.ru/" + rmTicket
        );
        issue.comment(
                "В связи с отсутствием активности в этом тикете, " +
                        "проект в Testenv'e, привязанный к этой RM компоненте, был остановлен. \n" +
                        "Запуск релизов более невозможен. \n" +
                        "Если у вас есть срочная необходимость выкатить релиз, " +
                        "вы можете временно включить Testenv-проект по ссылке: " + teDbUrl + "\n" +
                        "Однако если после этого вы не переедете в течение 2-х недель, " +
                        "ваша база будет выключена уже без возможности повторного включения.",
                assignee
        );
    }


    private boolean isNoReaction(Issue issue) {

        var comments = issue.getComments().toList();
        if (comments.isEmpty()) {
            return false;
        }
        var lastComment = comments.get(comments.size() - 1);
        return lastComment.getCreatedBy().getLogin().equals("robot-srch-releaser");

    }

    public List<MigratingTestenvProject> getActiveProjects() {
        return jdbcTemplate.query(
                "SELECT * FROM testenv_projects WHERE is_on = 1 AND db_profile IN ('release_machine', 'custom') " +
                        "AND (migration_status IS NULL OR migration_status != 'SKIP')",
                (rs, rowNum) ->
                        MigratingTestenvProject.builder()
                                .name(rs.getString("name"))
                                .dbProfile(rs.getString("db_profile"))
                                .ciTicket(rs.getString("ci_ticket"))
                                .rmTicket(rs.getString("rm_ticket"))
                                .rmComponent(rs.getString("rm_component"))
                                .migrationStatus(MigratingTestenvProject.MigrationStatus.of(rs.getString(
                                        "migration_status")))
                                .build()
        );
    }

    private void updateProject(MigratingTestenvProject project) {
        jdbcTemplate.update(
                "UPDATE testenv_projects " +
                        "SET ci_ticket = ?, rm_ticket = ?, rm_component = ?, migration_status = ? " +
                        "WHERE name = ?",
                project.getCiTicket(),
                project.getRmTicket(),
                project.getRmComponent(),
                project.getMigrationStatus().name(),
                project.getName()
        );
    }

    @Nullable
    private String findRmComponentInPochemutoDB(String database) {
        var result = jdbcTemplate.queryForList(
                "SELECT name FROM pochemuto_rm_databases WHERE trunk_db = ?",
                String.class,
                database
        );
        if (result.size() == 0) {
            return null;
        }
        if (result.size() > 1) {
            log.warn("Several RM component found for DB {}: {}", database, result);
        }
        return result.get(0);
    }


    public void processProjects() {
        var projects = getActiveProjects();

        log.info("Processing {} TE projects", projects.size());
        for (MigratingTestenvProject project : projects) {
            MigratingTestenvProject before = project.toBuilder().build();
            processProject(project);
            if (!before.equals(project)) {
                updateProject(project);
            }
        }
    }

    public void stopEmptyDatabases() {

        var projects = getActiveProjects();

        int total = 0;
        int stopped = 0;

        for (MigratingTestenvProject project : projects) {

            var jobs = testenvClient.getJobs(project.getName());

            boolean hasActiveJobs = false;

            for (TestenvJob job : jobs) {
                hasActiveJobs |= (job.getStatus() != TestenvJob.Status.STOPPED);
            }

            total++;
            if (!hasActiveJobs) {
                log.info("Found empty TE project, stopping: {}", project);
                testenvClient.stopProject(project.getName(), "No active jobs in DB");
                stopped++;
            }

            if (total % 20 == 0) {
                log.info("Total processed {}/{}, stopped {}", total, projects.size(), stopped);
            }
        }

        log.info("Stopping empty databases complete. Total processed {}, stopped {}", total, stopped);
    }

    private void processProject(MigratingTestenvProject project) {
        log.info("Processing {}", project.getName());
        if (isSkip(project)) {
            log.info("Skipping {}", project.getName());
            project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.SKIP);
            return;
        }

        if (project.isRm() || teDb2RmMigrationStatuses.containsKey(project.getName())) {
            processRMProject(project);
        }
        processCiWelcome(project);
    }

    private void processCiWelcome(MigratingTestenvProject project) {
        Issue issue = teDb2CiWelcomeIssue.get(project.getName());
        if (issue == null) {
            project.setCiTicket(null);
            if (project.getRmTicket() == null) {
                project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.MISSING);
            }
            return;
        }
        project.setCiTicket(issue.getKey());
        var status = issue.getStatus().getKey();
        switch (status) {
            case "closed" -> project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.INCONSISTENT);
            case "needAnalysis", "needInfo" -> project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.TODO);
            case "blocked" -> project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.BLOCKED);
            case "pendingDeveloperRelease" -> processProjectDeadline(project, issue);
            default -> throw new UnsupportedOperationException(
                    "Unknown status " + status + " for issue " + issue.getKey()
            );
        }

    }

    private void processProjectDeadline(MigratingTestenvProject project, Issue issue) {
        var jodaDeadline = issue.getDeadline().orElse((org.joda.time.LocalDate) null);
        if (jodaDeadline == null) {
            project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.TODO);
            return;
        }
        var deadline = LocalDate.ofYearDay(jodaDeadline.getYear(), jodaDeadline.getDayOfYear());

        if (deadline.isBefore(LocalDate.now(ZoneId.systemDefault()))) {
            project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.DEADLINE_EXCEEDED);
        } else {
            project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.WAITING_USER);
        }
    }

    private void processRMProject(MigratingTestenvProject project) {

        var rmStatus = teDb2RmMigrationStatuses.get(project.getName());


        if (rmStatus == null) {
            if (project.getName().endsWith("_graphs-trunk")) {
                project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.WAITING_USER);
                project.setRmTicket("RMDEV-2661");
                return;
            }

            log.warn("No found rm status for TE db {}", project.getName());
            project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.MISSING);
            return;
        }

        if (Strings.isNullOrEmpty(rmStatus.getTicket())) {
            log.warn("No RM ticket for rm status {}", rmStatus);
            project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.MISSING);
            return;
        }
        project.setRmComponent(rmStatus.getComponentName());
        project.setRmTicket(rmStatus.getTicket());
        project.setMigrationStatus(MigratingTestenvProject.MigrationStatus.TODO);
        log.info("Successfully process TM DB {} with RM component {}", project.getName(), rmStatus.getComponentName());
    }


//    @Nullable
//    private String getRmComponent(TestenvProject project) {
//        String possibleComponent = project.getName().replace("-trunk", "");
//
//        if (rmMigrationStatuses.containsKey(possibleComponent)) {
//            return possibleComponent;
//        }
//        return findRmComponentInPochemutoDB(project.getName());
//    }

    private boolean isSkip(MigratingTestenvProject project) {
        return isSkip(project, "-") || isSkip(project, "_r");
    }


    private boolean isSkip(MigratingTestenvProject project, String numberDelimiter) {
        String name = project.getName();
        int dashIndex = name.lastIndexOf(numberDelimiter);
        if (dashIndex <= 0) {
            return false;
        }
        name = name.substring(dashIndex + numberDelimiter.length());
        try {
            Integer.parseInt(name);
        } catch (NumberFormatException e) {
            return false;
        }
        return true;
    }

    //Табличка отсюда https://wiki.yandex-team.ru/releasemachine/rm-over-ci/migration-list/
    //Копируется руками
    private Map<String, RmMigrationStatus> getTeDBToMigrationStatuses() {
        var text = ResourceUtils.textResource("testenv/rm-migration-status.csv");
        var result = new HashMap<String, RmMigrationStatus>();

        for (String line : Splitter.on('\n').omitEmptyStrings().split(text)) {
            var parts = Splitter.on(',').trimResults().splitToList(line);

            RmMigrationStatus status = RmMigrationStatus.builder()
                    .componentName(parts.get(0))
                    .status(parts.get(1))
                    .ticket(getOrEmpty(parts, 8))
                    .trunkDb(getOrEmpty(parts, 9))
                    .build();

            if (status.getTrunkDb().isEmpty()) {
                log.warn("RM component without TE DB: {}", status);
                continue;
            }

            result.put(status.getTrunkDb(), status);
        }
        return result;
    }

    private Map<String, Issue> getTeDb2CiWelcomeIssues() {

        var result = new HashMap<String, Issue>();
        var issues = trackerSession.issues().find(SearchRequest.builder().queue("CIWELCOME").build())
                .toList();
        for (Issue issue : issues) {
            for (String tag : issue.getTags()) {
                if (!tag.startsWith(TAG_DB_PREFIX)) {
                    continue;
                }
                String db = tag.substring(TAG_DB_PREFIX.length());
                Issue previousIssue = result.put(db, issue);
                if (previousIssue != null) {
                    result.put(db, choseIssue(db, issue, previousIssue));
                }

            }
        }
        return result;
    }

    private Issue choseIssue(String db, Issue issue1, Issue issue2) {
        //Перепросим тикеты, что бы обойти кеши трекера при ручных правках
        issue1 = trackerSession.issues().get(issue1.getKey());
        issue2 = trackerSession.issues().get(issue2.getKey());
        if (isDuplicate(issue1)) {
            return issue2;
        }
        if (isDuplicate(issue2)) {
            return issue1;
        }
        throw new IllegalStateException(
                String.format(
                        "More than one issue to DB %s: %s, %s",
                        db, issue1.getKey(), issue2.getKey()
                )
        );
    }

    private boolean isDuplicate(Issue issue) {
        if (issue.getResolution().isEmpty()) {
            return false;
        }
        return issue.getResolution().get().getKey().equals("duplicate");
    }


    private String getOrEmpty(List<String> parts, int index) {
        if (parts.size() > index) {
            return parts.get(index);
        }
        return "";
    }


}
