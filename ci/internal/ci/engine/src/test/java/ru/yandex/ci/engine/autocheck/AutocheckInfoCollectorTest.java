package ru.yandex.ci.engine.autocheck;

import java.nio.file.Path;
import java.util.List;
import java.util.Optional;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.AutocheckConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.ReleaseTitleSource;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.engine.autocheck.config.AutocheckYamlService;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.pcm.PCMSelector;
import ru.yandex.ci.util.jackson.parse.ParseInfo;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;

class AutocheckInfoCollectorTest extends CommonTestBase {

    @MockBean
    private FastCircuitTargetsAutoResolver resolver;

    @MockBean
    private PCMSelector pcmSelector;

    @MockBean
    private AYamlService aYamlService;

    @MockBean
    private AutocheckYamlService autocheckYamlService;

    private AutocheckInfoCollector collector;

    @BeforeEach
    public void setup() {
        when(pcmSelector.selectPool(any(), any())).thenReturn("pool");
        when(autocheckYamlService.getLastConfigForRevision(any()))
                .thenReturn(AutocheckServiceTest.expectedConfigBundle());

        collector = new AutocheckInfoCollector(
                autocheckYamlService,
                aYamlService,
                resolver,
                ArcCommit.builder().build(),
                ArcCommit.builder().build(),
                pcmSelector,
                "dimello"
        );
    }

    @Test
    public void autoFastTargetIsTrueIfCalculated() throws Exception {
        when(resolver.getFastTarget(any())).thenReturn(Optional.of("module"));
        applyConfig(List.of());
        AutocheckLaunchConfig config = collector.getAutocheckLaunchConfig();
        assertThat(config.isAutodetectedFastCircuit()).isTrue();
        assertThat(config.getFastTargets()).isNotEmpty();
    }

    @Test
    public void autFastTargetIsFalseIfEmpty() throws Exception {
        applyConfig(List.of());
        AutocheckLaunchConfig config = collector.getAutocheckLaunchConfig();
        assertThat(config.isAutodetectedFastCircuit()).isFalse();
        assertThat(config.getFastTargets()).isEmpty();
    }


    @Test
    public void autoFastTargetIsFalseWhenManuallyDefined() throws Exception {
        List<String> targets = List.of("target");
        applyConfig(targets);
        AutocheckLaunchConfig result = collector.getAutocheckLaunchConfig();
        assertThat(result.isAutodetectedFastCircuit()).isFalse();
        assertThat(result.getFastTargets()).isNotEmpty();
    }

    private void applyConfig(List<String> targets) throws Exception {
        AutocheckConfig autoConfig = AutocheckConfig.builder()
                .fastTargets(targets)
                .build();
        CiConfig ciConfig = CiConfig.builder()
                .secret("secret")
                .runtime(RuntimeConfig.builder().sandbox(RuntimeSandboxConfig.builder().build()).build())
                .autocheck(autoConfig)
                .parseInfo(new ParseInfo("path"))
                .releaseTitleSource(ReleaseTitleSource.FLOW)
                .build();
        AYamlConfig config = new AYamlConfig("service", "title", ciConfig, null);
        when(aYamlService.getConfig(any(Path.class), any(CommitId.class))).thenReturn(config);
        Repo.ChangelistResponse.Change change = Repo.ChangelistResponse.Change.newBuilder()
                .setChange(Repo.ChangelistResponse.ChangeType.Modify)
                .setType(Shared.TreeEntryType.TreeEntryDir)
                .setPath("path")
                .build();
        collector.accept(change);
    }
}
