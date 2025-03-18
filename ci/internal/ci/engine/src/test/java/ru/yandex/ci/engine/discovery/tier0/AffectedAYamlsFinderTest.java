package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.ListMultimap;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyList;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Platform.LINUX;

class AffectedAYamlsFinderTest {

    private static final OrderedArcRevision REVISION = OrderedArcRevision.fromHash("commit-hash", ArcBranch.trunk(),
            1, 0);
    private static final int CACHE_SIZE = 1000;

    @Test
    void accept() {
        ArcService arcService = mock(ArcService.class);
        mockArcResponses(arcService, List.of("ci/a.yaml", "a.yaml"));

        var discoveredAYamlFilter = mock(AYamlFilterChecker.class);
        var acranumAYaml = CiProcessId.ofFlow(Path.of("arcanum/a.yaml"), "some-flow");
        when(discoveredAYamlFilter.findProcessesTriggeredByPathAndPlatform(any(), any(), any(), eq(false)))
                .thenReturn(
                        TriggeredProcesses.of(List.of(
                                new TriggeredProcesses.Triggered(
                                        TestData.RELEASE_PROCESS_ID,
                                        FilterConfig.builder()
                                                .discovery(FilterConfig.Discovery.GRAPH)
                                                .build()
                                ),
                                new TriggeredProcesses.Triggered(
                                        acranumAYaml,
                                        FilterConfig.builder()
                                                .discovery(FilterConfig.Discovery.GRAPH)
                                                .build()
                                )
                        ))
                );

        var consumer = new AffectedAYamlsFinder(
                OrderedArcRevision.fromHash("commit-hash", ArcBranch.trunk(), 1, 0),
                arcService, 1000, discoveredAYamlFilter, 10000
        );

        consumer.accept(LINUX, Path.of("ci/clients/arcanum"));
        consumer.accept(LINUX, Path.of("ci/clients/charts"));
        consumer.accept(LINUX, Path.of("ci/clients/old-ci"));
        consumer.accept(LINUX, Path.of("ci/clients/staff"));
        consumer.accept(LINUX, Path.of("dir-without-ayaml/lib"));
        consumer.processPendingBatchAndClearIt("limit reached");

        assertThat(consumer.getTriggered()).isEqualTo(
                Set.of(
                        new TriggeredProcesses.Triggered(
                                TestData.RELEASE_PROCESS_ID,
                                FilterConfig.builder()
                                        .discovery(FilterConfig.Discovery.GRAPH)
                                        .build()
                        ),
                        new TriggeredProcesses.Triggered(
                                acranumAYaml,
                                FilterConfig.builder()
                                        .discovery(FilterConfig.Discovery.GRAPH)
                                        .build()
                        )
                )
        );
    }

    private static void mockArcResponses(ArcService arcService, List<String> existingAYamls) {
        Set<Path> existingAYamlSet = existingAYamls.stream().map(Path::of).collect(Collectors.toSet());

        doReturn(true).when(arcService).isFileExists(
                argThat(existingAYamlSet::contains),
                argThat(it -> "commit-hash".equals(it.getCommitId()))
        );
        doReturn(false).when(arcService).isFileExists(
                argThat(path -> !existingAYamlSet.contains(path)),
                argThat(it -> "commit-hash".equals(it.getCommitId()))
        );
    }

    @Test
    void accept_shouldProcessAllAYamlsFromLeaftToRootAndNotOnlyFirstMet() {
        var arcService = mock(ArcService.class);
        mockArcResponses(arcService, List.of("alice/a.yaml", "alice/cuttlefish/a.yaml"));

        var aYamlFilterChecker = mock(AYamlFilterChecker.class);
        doAnswer(args -> TriggeredProcesses.empty()).when(aYamlFilterChecker)
                .findProcessesTriggeredByPathAndPlatform(any(), any(), anyList(), eq(false));

        var pathMatcher = new AffectedAYamlsFinder(
                OrderedArcRevision.fromHash("commit-hash", ArcBranch.trunk(), 1, 0),
                arcService, 1000, aYamlFilterChecker, 10
        );
        pathMatcher.accept(LINUX, Path.of("alice/cuttlefish/tools"));

        // alice/cuttlefish/tools/a.yaml doesn't exist
        verify(arcService, times(1)).isFileExists(eq(Path.of("alice/cuttlefish/tools/a.yaml")), any());
        // alice/cuttlefish/tools/a.yaml exists
        verify(arcService, times(1)).isFileExists(eq(Path.of("alice/cuttlefish/a.yaml")), any());
        // alice/cuttlefish/tools/a.yaml exists
        verify(arcService, times(1)).isFileExists(eq(Path.of("alice/a.yaml")), any());
    }

    @Test
    void collectTriggeredAYamls_shouldProcessBatchWhenBatchSizeReachedTheLimit() {
        ListMultimap<String, String> aYamlToYaMakes = ArrayListMultimap.create();
        AYamlFilterChecker aYamlFilterChecker = mockAYamlFilterChecker(aYamlToYaMakes);

        var consumer = new AffectedAYamlsFinder(
                OrderedArcRevision.fromHash("commit-hash", ArcBranch.trunk(), 1, 0),
                mock(ArcService.class), 1000, aYamlFilterChecker, 2
        );

        // `aYamlToYaMakes` should be empty cause batch size hasn't been reached
        consumer.collectTriggeredAYamls(LINUX, Path.of("ci/01/ya.make"), Path.of("ci/a.yaml"));
        assertThat(aYamlToYaMakes.asMap()).isEqualTo(Map.of());

        // `aYamlToYaMakes` should be empty cause batch size hasn't been reached
        consumer.collectTriggeredAYamls(LINUX, Path.of("ci/02/ya.make"), Path.of("ci/a.yaml"));
        assertThat(aYamlToYaMakes.asMap()).isEqualTo(Map.of());

        // should process collected `foundAYamls` and `affectedYaMakes`, but without new ("ci/03/ya.make", "ci/a.yaml")
        // new pair ("ci/03/ya.make", "ci/a.yaml") will be processed in the next batch
        consumer.collectTriggeredAYamls(LINUX, Path.of("ci/03/ya.make"), Path.of("ci/a.yaml"));
        assertThat(aYamlToYaMakes.asMap()).isEqualTo(Map.of(
                "ci/a.yaml", List.of("ci/01/ya.make", "ci/02/ya.make")
        ));
    }

    @Test
    void collectTriggeredAYamls_shouldNotProcessBatchWhenNewAYamlPathsDiffersFromOld() {
        ListMultimap<String, String> aYamlToYaMakes = ArrayListMultimap.create();
        AYamlFilterChecker aYamlFilterChecker = mockAYamlFilterChecker(aYamlToYaMakes);

        var consumer = new AffectedAYamlsFinder(
                OrderedArcRevision.fromHash("commit-hash", ArcBranch.trunk(), 1, 0),
                mock(ArcService.class), 1000, aYamlFilterChecker, 2
        );

        // `aYamlToYaMakes` should be empty cause batch size hasn't been reached
        consumer.collectTriggeredAYamls(LINUX, Path.of("ci/01/ya.make"), Path.of("ci/a.yaml"));
        assertThat(aYamlToYaMakes.asMap()).isEqualTo(Map.of());

        // `aYamlToYaMakes` should be empty cause batch size hasn't been reached
        consumer.collectTriggeredAYamls(LINUX, Path.of("ci/02/ya.make"), Path.of("a.yaml"));
        assertThat(aYamlToYaMakes.asMap()).isEqualTo(Map.of());

        // should process collected `foundAYamls` and `affectedYaMakes`, but without new ("ci/03/ya.make", "ci/a.yaml")
        // new pair ("ci/03/ya.make", "ci/a.yaml") will be processed in the next batch
        consumer.collectTriggeredAYamls(LINUX, Path.of("ci/03/ya.make"), Path.of("arcanum/a.yaml"));
        assertThat(aYamlToYaMakes.asMap()).isEqualTo(Map.of(
                "ci/a.yaml", List.of("ci/01/ya.make"),
                "a.yaml", List.of("ci/02/ya.make")
        ));
    }

    @Test
    void passToAllFilters() {
        var aYamlFilterChecker = mock(AYamlFilterChecker.class);
        when(aYamlFilterChecker.findProcessesTriggeredByPathAndPlatform(any(), any(), anyList(), eq(false)))
                .thenReturn(TriggeredProcesses.empty());

        var consumer = new AffectedAYamlsFinder(
                REVISION, mock(ArcService.class), CACHE_SIZE, aYamlFilterChecker, 1);

        consumer.collectTriggeredAYamls(LINUX, Path.of("quasar/infra/tasklets/ya.make"), Path.of("quasar/infra" +
                "/tasklets/a.yaml"));
        consumer.collectTriggeredAYamls(LINUX, Path.of("sandbox/projects/quasar/platform/ya.make"), Path.of("quasar" +
                "/infra/tasklets/a.yaml"));
    }

    private static AYamlFilterChecker mockAYamlFilterChecker(ListMultimap<String, String> aYamlToYaMakes) {
        var aYamlFilterChecker = mock(AYamlFilterChecker.class);
        doAnswer(args -> {
            Path aYamlPath = args.getArgument(0);
            List<String> affectedPaths = args.getArgument(2);
            aYamlToYaMakes.putAll(aYamlPath.toString(), affectedPaths);

            return TriggeredProcesses.of(List.of(
                    new TriggeredProcesses.Triggered(
                            TestData.RELEASE_PROCESS_ID,
                            FilterConfig.builder()
                                    .discovery(FilterConfig.Discovery.GRAPH)
                                    .build()
                    )));
        }).when(aYamlFilterChecker).findProcessesTriggeredByPathAndPlatform(any(), any(), anyList(), eq(false));
        return aYamlFilterChecker;
    }

}
