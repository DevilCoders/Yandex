package ru.yandex.ci.engine.discovery;


import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;

class TriggeredProcessesTest {


    @Test
    void mergeEmpty() {
        var processes = TriggeredProcesses.empty();
        processes.merge(TriggeredProcesses.empty());

        assertThat(processes).isEqualTo(TriggeredProcesses.empty());
    }

    @Test
    void mergeFlowIds() {
        FilterConfig filter1 = FilterConfig.builder().stQueue("ST_1").build();
        FilterConfig filter2 = FilterConfig.builder().stQueue("ST_2").build();

        var flowId1 = CiProcessId.ofFlow(TestData.TRIGGER_ON_COMMIT_AYAML_PATH, "flowId1");
        var flowId2 = CiProcessId.ofFlow(TestData.TRIGGER_ON_COMMIT_AYAML_PATH, "flowId2");

        var oldTriggeredProcesses = TriggeredProcesses.of(List.of(
                new TriggeredProcesses.Triggered(flowId1, filter1)
        ));

        oldTriggeredProcesses = oldTriggeredProcesses.merge(TriggeredProcesses.of(List.of(
                new TriggeredProcesses.Triggered(flowId1, filter2)
        )));

        oldTriggeredProcesses = oldTriggeredProcesses.merge(TriggeredProcesses.of(List.of(
                new TriggeredProcesses.Triggered(flowId2, filter1)
        )));

        assertThat(oldTriggeredProcesses.getActions()).isEqualTo(
                Map.of(
                        flowId1, Set.of(filter1, filter2),
                        flowId2, Set.of(filter1)
                )
        );
    }

    @Test
    void mergeReleaseIds() {
        FilterConfig filter1 = FilterConfig.builder().stQueue("ST_1").build();
        FilterConfig filter2 = FilterConfig.builder().stQueue("ST_2").build();

        CiProcessId releaseId1 = TestData.RELEASE_PROCESS_ID;
        CiProcessId releaseId2 = TestData.SIMPLEST_RELEASE_PROCESS_ID;

        var oldTriggeredProcesses = TriggeredProcesses.of(List.of(
                new TriggeredProcesses.Triggered(releaseId1, filter1)
        ));

        oldTriggeredProcesses = oldTriggeredProcesses.merge(TriggeredProcesses.of(List.of(
                new TriggeredProcesses.Triggered(releaseId1, filter2)
        )));

        oldTriggeredProcesses = oldTriggeredProcesses.merge(TriggeredProcesses.of(List.of(
                new TriggeredProcesses.Triggered(releaseId2, filter2)
        )));

        assertThat(oldTriggeredProcesses.getReleases()).isEqualTo(Map.of(
                releaseId1, Set.of(filter1, filter2),
                releaseId2, Set.of(filter2)
        ));
    }


}
