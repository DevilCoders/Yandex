package ru.yandex.ci.core.project;

import java.nio.file.Path;
import java.util.Map;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.LaunchTable.CountByProcessIdAndStatus;

import static org.assertj.core.api.Assertions.assertThat;

class ProjectCountersTest {

    private static final CiProcessId RELEASE_1 = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "release-1");
    private static final CiProcessId RELEASE_2 = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "release-2");

    @Test
    void add() {
        var counters = new ProjectCounters();
        assertThat(counters.getProcessIdBranchMap()).isEqualTo(Map.of());
        assertThat(counters.getProcessIdMap()).isEqualTo(Map.of());

        counters.add(new CountByProcessIdAndStatus(RELEASE_1.asString(), Status.RUNNING, "trunk", 2));
        assertThat(counters.getProcessIdBranchMap()).isEqualTo(Map.of(
                ProcessIdBranch.of(RELEASE_1, "trunk"), Map.of(Status.RUNNING, 2L)
        ));
        assertThat(counters.getProcessIdMap()).isEqualTo(Map.of(
                RELEASE_1, Map.of(Status.RUNNING, 2L)
        ));

        counters.add(new CountByProcessIdAndStatus(RELEASE_1.asString(), Status.RUNNING, "releases/ci/1", 1));
        assertThat(counters.getProcessIdBranchMap()).isEqualTo(Map.of(
                ProcessIdBranch.of(RELEASE_1, "trunk"), Map.of(Status.RUNNING, 2L),
                ProcessIdBranch.of(RELEASE_1, "releases/ci/1"), Map.of(Status.RUNNING, 1L)
        ));
        assertThat(counters.getProcessIdMap()).isEqualTo(Map.of(
                RELEASE_1, Map.of(Status.RUNNING, 3L)
        ));

        counters.add(new CountByProcessIdAndStatus(RELEASE_1.asString(), Status.FAILURE, "trunk", 1));
        assertThat(counters.getProcessIdBranchMap()).isEqualTo(Map.of(
                ProcessIdBranch.of(RELEASE_1, "trunk"), Map.of(
                        Status.RUNNING, 2L,
                        Status.FAILURE, 1L
                ),
                ProcessIdBranch.of(RELEASE_1, "releases/ci/1"), Map.of(Status.RUNNING, 1L)
        ));
        assertThat(counters.getProcessIdMap()).isEqualTo(Map.of(
                RELEASE_1, Map.of(
                        Status.RUNNING, 3L,
                        Status.FAILURE, 1L
                )
        ));

        counters.add(new CountByProcessIdAndStatus(RELEASE_2.asString(), Status.RUNNING, "trunk", 1));
        assertThat(counters.getProcessIdBranchMap()).isEqualTo(Map.of(
                ProcessIdBranch.of(RELEASE_1, "trunk"), Map.of(
                        Status.RUNNING, 2L,
                        Status.FAILURE, 1L
                ),
                ProcessIdBranch.of(RELEASE_1, "releases/ci/1"), Map.of(Status.RUNNING, 1L),
                ProcessIdBranch.of(RELEASE_2, "trunk"), Map.of(Status.RUNNING, 1L)
        ));
        assertThat(counters.getProcessIdMap()).isEqualTo(Map.of(
                RELEASE_1, Map.of(
                        Status.RUNNING, 3L,
                        Status.FAILURE, 1L
                ),
                RELEASE_2, Map.of(Status.RUNNING, 1L)
        ));
    }

}
