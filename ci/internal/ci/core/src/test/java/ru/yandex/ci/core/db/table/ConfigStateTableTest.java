package ru.yandex.ci.core.db.table;

import java.nio.file.Path;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.stream.Stream;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.ConfigState.Status;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class ConfigStateTableTest extends CommonYdbTestBase {

    @Test
    void upsertIfNewer() {
        assertThatThrownBy(() -> db.currentOrReadOnly(() -> db.configStates().get(TestData.CONFIG_PATH_ABC)))
                .isInstanceOf(NoSuchElementException.class);

        Instant now = Instant.ofEpochMilli(System.currentTimeMillis());

        ConfigState r1State = ConfigState.builder()
                .configPath(TestData.CONFIG_PATH_ABC)
                .status(Status.INVALID)
                .lastRevision(TestData.TRUNK_R2)
                .lastValidRevision(TestData.TRUNK_R2)
                .project("ci")
                .updated(now)
                .created(now)
                .build();

        ConfigState r2State = r1State.toBuilder()
                .lastRevision(TestData.TRUNK_R3)
                .lastValidRevision(TestData.TRUNK_R3)
                .status(Status.OK)
                .actions(List.of(
                        ActionConfigState.builder().flowId("flow1").title("Лучший флоу").build())
                )
                .releases(List.of(
                        ReleaseConfigState.builder().releaseId("my-release").title("Релизище!!!").build()
                ))
                .build();

        ConfigState r3State = r2State.toBuilder()
                .lastRevision(TestData.TRUNK_R4)
                .lastValidRevision(TestData.TRUNK_R3)
                .status(Status.INVALID_PREVIOUS_VALID)
                .build();

        db.currentOrTx(() -> db.configStates().upsertIfNewer(r2State));
        assertThat(db.currentOrReadOnly(() -> db.configStates().get(TestData.CONFIG_PATH_ABC))).isEqualTo(r2State);

        db.currentOrTx(() -> db.configStates().upsertIfNewer(r1State));
        assertThat(db.currentOrReadOnly(() -> db.configStates().get(TestData.CONFIG_PATH_ABC))).isEqualTo(r2State);

        db.currentOrTx(() -> db.configStates().upsertIfNewer(r3State));
        assertThat(db.currentOrReadOnly(() -> db.configStates().get(TestData.CONFIG_PATH_ABC))).isEqualTo(r3State);

        db.currentOrTx(() -> db.configStates().upsertIfNewer(r2State));
        assertThat(db.currentOrReadOnly(() -> db.configStates().get(TestData.CONFIG_PATH_ABC))).isEqualTo(r3State);

    }

    @Test
    void findByProject() {
        Instant now = Instant.ofEpochMilli(System.currentTimeMillis());

        ConfigState stateAbd = ConfigState.builder()
                .configPath(TestData.CONFIG_PATH_ABD)
                .status(Status.INVALID)
                .lastRevision(TestData.TRUNK_R2)
                .lastValidRevision(TestData.TRUNK_R2)
                .project("ci")
                .updated(now)
                .created(now)
                .actions(List.of(
                        ActionConfigState.builder().flowId("flow1").title("Лучший флоу").build())
                )
                .releases(List.of(
                        ReleaseConfigState.builder().releaseId("my-release").title("Релизище!!!").build()
                ))
                .build();

        ConfigState stateAbe = stateAbd.toBuilder()
                .configPath(TestData.CONFIG_PATH_ABE)
                .build();

        ConfigState stateAbf = stateAbd.toBuilder()
                .configPath(TestData.CONFIG_PATH_ABF)
                .project("not-ci")
                .build();

        ConfigState stateAbj = stateAbd.toBuilder()
                .configPath(TestData.CONFIG_PATH_ABJ)
                .project(null)
                .build();

        db.currentOrTx(() -> {
            db.configStates().upsertIfNewer(stateAbd);
            db.configStates().upsertIfNewer(stateAbe);
            db.configStates().upsertIfNewer(stateAbf);
            db.configStates().upsertIfNewer(stateAbj);
        });

        db.currentOrReadOnly(() -> {
            assertThat(db.configStates().findByProject("ci", true))
                    .isEqualTo(List.of(stateAbd, stateAbe));
            assertThat(db.configStates().findByProject("ci", false))
                    .isEmpty();
        });
    }

    @Test
    void listProjects() {
        List<String> projects = new ArrayList<>();
        db.currentOrTx(
                () -> {
                    for (int i = 0; i < 42; i++) {
                        String project = String.format("project%03d", i);
                        projects.add(project);
                        for (int j = 0; j <= i % 3; j++) {
                            ConfigState state = ConfigState.builder()
                                    .configPath(Path.of(project, "config" + j))
                                    .project(project)
                                    .status(Status.OK)
                                    .build();
                            db.configStates().upsertIfNewer(state);
                        }
                    }
                }
        );

        db.currentOrReadOnly(() -> {
            assertThat(db.configStates().listProjects(null, projects.get(21), false, 1000))
                    .isEqualTo(projects.subList(22, projects.size()));

            assertThat(db.configStates().listProjects(null, projects.get(21), true, 1000))
                    .isEqualTo(projects.subList(22, projects.size()));

            assertThat(db.configStates().listProjects(null, projects.get(21), false, 7))
                    .isEqualTo(projects.subList(22, 29));
        });
    }

    @Test
    void listProjects_withExcludedStatuses() {
        db.currentOrTx(() ->
                List.of(
                        ConfigState.builder()
                                .configPath(Path.of("project-A", "config-A"))
                                .project("project-A")
                                .status(Status.OK)
                                .build(),
                        ConfigState.builder()
                                .configPath(Path.of("project-B", "config-B"))
                                .project("project-B")
                                .status(Status.DELETED)
                                .build(),
                        ConfigState.builder()
                                .configPath(Path.of("project-C", "config-C"))
                                .project("project-C")
                                .status(Status.INVALID_PREVIOUS_VALID)
                                .build(),
                        ConfigState.builder()
                                .configPath(Path.of("not-ci-project", "not-ci-config"))
                                .project("not-ci-project")
                                .status(Status.INVALID)
                                .build(),
                        ConfigState.builder()
                                .configPath(Path.of("project-D", "config-D"))
                                .project("project-D")
                                .status(Status.DRAFT)
                                .build()
                ).forEach(it -> db.configStates().save(it)));

        db.currentOrReadOnly(() -> {
            assertThat(db.configStates().listProjects(null, null, List.of(), 1000))
                    .isEqualTo(List.of("not-ci-project", "project-A", "project-B", "project-C", "project-D"));
            assertThat(db.configStates().listProjects(null, null, List.of(Status.INVALID), 1000))
                    .isEqualTo(List.of("project-A", "project-B", "project-C", "project-D"));
        });
    }

    @Test
    void created() {
        Instant created = Instant.ofEpochMilli(System.currentTimeMillis());
        Instant updated = created.plusSeconds(42);

        ConfigState state1 = ConfigState.builder()
                .configPath(TestData.CONFIG_PATH_ABC)
                .project("ci")
                .created(created)
                .updated(updated)
                .lastRevision(TestData.TRUNK_R2)
                .lastValidRevision(TestData.TRUNK_R2)
                .build();

        db.currentOrTx(() -> db.configStates().upsertIfNewer(state1));

        ConfigState state2 = ConfigState.builder()
                .configPath(TestData.CONFIG_PATH_ABC)
                .project("ci")
                .created(updated)
                .updated(updated)
                .lastRevision(TestData.TRUNK_R3)
                .lastValidRevision(TestData.TRUNK_R3)
                .build();

        db.currentOrTx(() -> db.configStates().upsertIfNewer(state2));

        assertThat(db.currentOrReadOnly(() -> db.configStates().get(TestData.CONFIG_PATH_ABC)))
                .isEqualTo(state2.withCreated(created));

    }

    @Test
    void findBySameProjectDifferentStatuses() {
        var project = "test";
        var states = new HashMap<Status, ConfigState>();
        db.currentOrTx(() -> {
            for (var status : Status.values()) {
                ConfigState state = ConfigState.builder()
                        .configPath(Path.of(project, status.name()))
                        .project(project)
                        .status(status)
                        .build();
                states.put(status, state);
                db.configStates().upsertIfNewer(state);
            }
        });

        db.currentOrReadOnly(() -> {

            // findByProject
            assertThat(db.configStates().findByProject(project, false))
                    .isEqualTo(getStates(states, EnumSet.of(Status.DELETED, Status.INVALID)));

            assertThat(db.configStates().findByProject(project, true))
                    .isEqualTo(getStates(states, EnumSet.of(Status.DELETED)));

            // findAllVisible
            assertThat(db.configStates().findAllVisible(false))
                    .isEqualTo(getStates(states,
                            EnumSet.of(Status.DELETED, Status.DRAFT, Status.NOT_CI, Status.INVALID)));

            assertThat(db.configStates().findAllVisible(true))
                    .isEqualTo(getStates(states, EnumSet.of(Status.DELETED, Status.DRAFT, Status.NOT_CI)));

            // findVisiblePaths
            assertThat(db.configStates().findVisiblePaths())
                    .isEqualTo(toPaths(getStates(states, EnumSet.of(Status.DELETED, Status.DRAFT, Status.NOT_CI))));

            // listProjects
            assertThat(db.configStates().listProjects(null, null, false, 1000))
                    .isEqualTo(toProjects(getStates(states, EnumSet.of(Status.DELETED, Status.INVALID))));

            assertThat(db.configStates().listProjects(null, null, true, 1000))
                    .isEqualTo(toProjects(getStates(states, EnumSet.of(Status.DELETED))));

            // countProjects
            assertThat(db.configStates().countProjects(null, false))
                    .isEqualTo(1);

            assertThat(db.configStates().countProjects(null, true))
                    .isEqualTo(1);
        });

    }

    @Test
    void findByDifferentProjectDifferentStatuses() {
        var states = new HashMap<Status, ConfigState>();
        db.currentOrTx(() -> {
            for (var status : Status.values()) {
                var project = status.name();
                ConfigState state = ConfigState.builder()
                        .configPath(Path.of(project, status.name()))
                        .project(project)
                        .status(status)
                        .build();
                states.put(status, state);
                db.configStates().upsertIfNewer(state);
            }
        });

        db.currentOrReadOnly(() -> {

            // findByProject
            for (var status : withoutStatuses(EnumSet.of(Status.DELETED, Status.INVALID))) {
                assertThat(db.configStates().findByProject(status.name(), false))
                        .isEqualTo(List.of(states.get(status)));
            }
            for (var status : EnumSet.of(Status.DELETED, Status.INVALID)) {
                assertThat(db.configStates().findByProject(status.name(), false))
                        .isEmpty();
            }

            for (var status : withoutStatuses(EnumSet.of(Status.DELETED))) {
                assertThat(db.configStates().findByProject(status.name(), true))
                        .isEqualTo(List.of(states.get(status)));
            }
            for (var status : EnumSet.of(Status.DELETED)) {
                assertThat(db.configStates().findByProject(status.name(), true))
                        .isEmpty();
            }


            // findAllVisible
            assertThat(db.configStates().findAllVisible(false))
                    .isEqualTo(getStates(states,
                            EnumSet.of(Status.DELETED, Status.DRAFT, Status.NOT_CI, Status.INVALID)));

            assertThat(db.configStates().findAllVisible(true))
                    .isEqualTo(getStates(states, EnumSet.of(Status.DELETED, Status.DRAFT, Status.NOT_CI)));

            // findVisiblePaths
            assertThat(db.configStates().findVisiblePaths())
                    .isEqualTo(toPaths(getStates(states, EnumSet.of(Status.DELETED, Status.DRAFT, Status.NOT_CI))));

            // listProjects
            assertThat(db.configStates().listProjects(null, null, false, 1000))
                    .isEqualTo(toProjects(getStates(states, EnumSet.of(Status.DELETED, Status.INVALID))));

            assertThat(db.configStates().listProjects(null, null, true, 1000))
                    .isEqualTo(toProjects(getStates(states, EnumSet.of(Status.DELETED))));

            // countProjects
            var totalLen = Status.values().length;
            assertThat(db.configStates().countProjects(null, false))
                    .isEqualTo(totalLen - 2); // 4

            assertThat(db.configStates().countProjects(null, true))
                    .isEqualTo(totalLen - 1); // 5
        });

    }

    private static List<Status> withoutStatuses(EnumSet<Status> excludeStatuses) {
        return Stream.of(Status.values())
                .filter(e -> !excludeStatuses.contains(e))
                .toList();
    }

    private static List<ConfigState> getStates(Map<Status, ConfigState> configs, EnumSet<Status> excludeStates) {
        return configs.entrySet().stream()
                .filter(e -> !excludeStates.contains(e.getKey()))
                .map(Map.Entry::getValue)
                .sorted(Comparator.comparing(x -> x.getId().getConfigPath()))
                .toList();
    }

    private static List<String> toPaths(List<ConfigState> states) {
        return states.stream()
                .map(ConfigState::getConfigPath)
                .map(Path::toString)
                .distinct()
                .toList();
    }

    private static List<String> toProjects(List<ConfigState> states) {
        return states.stream()
                .map(ConfigState::getProject)
                .filter(Objects::nonNull)
                .distinct()
                .toList();
    }
}
