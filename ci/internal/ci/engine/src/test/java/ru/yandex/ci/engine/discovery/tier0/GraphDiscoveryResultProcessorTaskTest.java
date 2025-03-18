package ru.yandex.ci.engine.discovery.tier0;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import org.eclipse.jetty.io.RuntimeIOException;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.Resources;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.discovery.ConfigChange;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_RESULTS_PROCESSED;

@ContextConfiguration(classes = {
        GraphDiscoveryResultProcessorTaskTest.Config.class
})
public class GraphDiscoveryResultProcessorTaskTest extends EngineTestBase {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private static final CiProcessId PROCESS_ID = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "sawmill-release");
    private static final CiProcessId PROCESS_ID_2 = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "sawmill-release-2");

    private static final long SANDBOX_TASK_ID = 1L;
    private static final long SANDBOX_RESOURCE_ID = 2L;
    private static final Instant NOW = Instant.parse("2023-01-02T10:00:00.000Z");

    @Autowired
    ConfigDiscoveryDirCache configDiscoveryDirCache;

    @Autowired
    GraphDiscoveryResultProcessorTask graphDiscoveryResultProcessorTask;

    @Autowired
    ResultProcessorParameters graphDiscoveryResultProcessorParameters;

    @Test
    public void graphTriggersAYamlByAbsPath() throws Exception {
        discovery(TestData.TRUNK_COMMIT_2);
        verifyConfigDiscoveryDirCache(Map.of(
                "contrib/abs-path-filter", "ci/a.yaml"
        ));
        saveGraphDiscoveryTaskInDatabase(TestData.TRUNK_R2, TestData.TRUNK_R1);

        var affectedTargets = List.of("contrib/abs-path-filter/some/dir");
        mockSandboxServerResponses(affectedTargets);

        clock.setTime(NOW);
        graphDiscoveryResultProcessorTask.processGraphResults(SANDBOX_TASK_ID);

        verifyCommitIsDiscoveredByGraph(PROCESS_ID, TestData.TRUNK_R2);
        verifyGraphDiscoveryTaskStatus();
    }

    @Test
    public void graphTriggersAYamlBySubPath() throws Exception {
        discovery(TestData.TRUNK_COMMIT_3);
        verifyConfigDiscoveryDirCache(Map.of());
        saveGraphDiscoveryTaskInDatabase(TestData.TRUNK_R3, TestData.TRUNK_R1);

        var affectedTargets = List.of("ci/internal/docs");
        mockSandboxServerResponses(affectedTargets);

        clock.setTime(NOW);
        graphDiscoveryResultProcessorTask.processGraphResults(SANDBOX_TASK_ID);

        verifyCommitIsDiscoveredByGraph(PROCESS_ID, TestData.TRUNK_R3);
        verifyGraphDiscoveryTaskStatus();
    }

    @Test
    public void graphAffectsAbsPathAndNotAbsPath() throws Exception {
        discovery(TestData.TRUNK_COMMIT_4);
        verifyConfigDiscoveryDirCache(Map.of(
                "contrib/abs-path-filter", "ci/a.yaml"
        ));
        saveGraphDiscoveryTaskInDatabase(TestData.TRUNK_R4, TestData.TRUNK_R1);

        var affectedTargets = List.of(
                "contrib/abs-path-filter/some/dir",
                "contrib/abs-path-filter/not-abs-path-filter/another/dir"
        );
        mockSandboxServerResponses(affectedTargets);

        clock.setTime(NOW);
        graphDiscoveryResultProcessorTask.processGraphResults(SANDBOX_TASK_ID);

        verifyCommitIsDiscoveredByGraph(PROCESS_ID, TestData.TRUNK_R4);
        verifyGraphDiscoveryTaskStatus();
    }

    @Test
    public void twoConfigs_oneWithAbsPaths_anotherWithSubPath_shouldTriggerOnlyWithAbsPaths() throws Exception {
        discovery(TestData.TRUNK_COMMIT_5);
        verifyConfigDiscoveryDirCache(Map.of(
                "contrib/abs-path-filter", "ci/a.yaml"
        ));
        saveGraphDiscoveryTaskInDatabase(TestData.TRUNK_R5, TestData.TRUNK_R1);

        var affectedTargets = List.of(
                "contrib/abs-path-filter/some/dir",
                "contrib/abs-path-filter/not-abs-path-filter/another/dir"
        );
        mockSandboxServerResponses(affectedTargets);

        clock.setTime(NOW);
        graphDiscoveryResultProcessorTask.processGraphResults(SANDBOX_TASK_ID);

        verifyCommitIsDiscoveredByGraph(PROCESS_ID, TestData.TRUNK_R5);
        verifyNoCommitDiscovered(PROCESS_ID_2, TestData.TRUNK_R5);
        verifyGraphDiscoveryTaskStatus();
    }

    private void saveGraphDiscoveryTaskInDatabase(
            OrderedArcRevision rightRevision,
            OrderedArcRevision leftRevision
    ) {
        var taskIsGoingToBeProcessed = GraphDiscoveryTask
                .graphEvaluationStarted(
                        rightRevision.toRevision(),
                        leftRevision.toRevision(),
                        rightRevision.getNumber(),
                        leftRevision.getNumber(),
                        SANDBOX_TASK_ID,
                        Set.of(GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY),
                        SandboxTaskStatus.EXECUTING.name(),
                        Instant.parse("2020-01-02T10:00:00.000Z")
                )
                .toBuilder()
                .sandboxStatus(SandboxTaskStatus.SUCCESS.name())
                .sandboxResultResourceId(SANDBOX_RESOURCE_ID)
                .graphEvaluationFinishedAt(Instant.parse("2021-01-02T10:00:00.000Z"))
                .discoveryStartedAt(Instant.parse("2022-01-02T10:00:00.000Z"))
                .status(GraphDiscoveryTask.Status.GRAPH_RESULTS_PROCESSING)
                .build();
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(taskIsGoingToBeProcessed));
    }

    private void mockSandboxServerResponses(List<String> affectedTargets) throws JsonProcessingException {
        var resourceType = graphDiscoveryResultProcessorParameters.getSandboxResultResourceType();
        doReturn(
                new Resources(List.of(
                        ResourceInfo.builder()
                                .id(SANDBOX_RESOURCE_ID)
                                .fileName("affected_targets.json")
                                .type(resourceType)
                                .build()
                ))
        ).when(sandboxClient).getTaskResources(eq(SANDBOX_TASK_ID), eq(resourceType));

        doReturn(new InputStreamResource(
                new ByteArrayInputStream(
                        MAPPER.writeValueAsBytes(Map.of(
                                "linux", affectedTargets
                        ))
                )
        )).when(proxySandboxClient).downloadResource(SANDBOX_RESOURCE_ID);
    }

    private void verifyConfigDiscoveryDirCache(Map<String, String> prefixPathToConfig) {
        configDiscoveryDirCache.flushCaches();
        assertThat(configDiscoveryDirCache.getPrefixPathToConfigMap())
                .isEqualTo(prefixPathToConfig);
    }

    private void verifyCommitIsDiscoveredByGraph(CiProcessId processId, OrderedArcRevision revision) {
        db.currentOrReadOnly(() -> {
            assertThat(
                    db.discoveredCommit().findCommit(processId, revision)
                            .orElseThrow()
                            .getState()
            ).isEqualTo(
                    DiscoveredCommitState.builder()
                            .configChange(new ConfigChange(ConfigChangeType.NONE))
                            .graphDiscovery(true)
                            .build()
            );
        });
    }

    private void verifyNoCommitDiscovered(CiProcessId processId, OrderedArcRevision revision) {
        db.currentOrReadOnly(() -> {
            assertThat(
                    db.discoveredCommit().findCommit(processId, revision)
                            .map(DiscoveredCommit::getState)
            ).isEmpty();
        });
    }

    private void verifyGraphDiscoveryTaskStatus() {
        db.currentOrReadOnly(() -> {
            var task = db.graphDiscoveryTaskTable().findBySandboxTaskId(SANDBOX_TASK_ID).orElseThrow();
            assertThat(task.getSandboxResultResourceId()).isEqualTo(SANDBOX_RESOURCE_ID);
            assertThat(task.getStatus()).isEqualTo(GRAPH_RESULTS_PROCESSED);
            assertThat(task.getDiscoveryFinishedAt()).isEqualTo(NOW);
        });
    }


    @Configuration
    public static class Config {

        @Bean
        public ArcService arcService() {
            return new ArcServiceStub(
                    "test-repos/graph-discovery-result-processor",
                    TestData.TRUNK_COMMIT_2,
                    TestData.TRUNK_COMMIT_3.withParent(TestData.TRUNK_COMMIT_1),
                    TestData.TRUNK_COMMIT_4.withParent(TestData.TRUNK_COMMIT_1),
                    TestData.TRUNK_COMMIT_5.withParent(TestData.TRUNK_COMMIT_1)
            );
        }

    }

    @RequiredArgsConstructor
    private static class InputStreamResource implements ProxySandboxClient.CloseableResource {

        @Nonnull
        private final InputStream inputStream;

        @Override
        public InputStream getStream() {
            return inputStream;
        }

        @Override
        public void close() {
            try {
                inputStream.close();
            } catch (IOException e) {
                throw new RuntimeIOException(e);
            }
        }
    }

}
