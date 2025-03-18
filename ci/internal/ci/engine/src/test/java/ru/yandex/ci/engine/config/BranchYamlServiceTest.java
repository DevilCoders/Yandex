package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.util.List;
import java.util.Optional;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mockito;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.eq;

@ExtendWith(MockitoExtension.class)
@ContextConfiguration(classes = {
        ArcClientTestConfig.class,
})
class BranchYamlServiceTest extends CommonTestBase {

    @MockBean
    private ConfigurationService configurationService;

    private BranchYamlService service;

    @BeforeEach
    void setUp() {
        ArcService arcService = new ArcServiceStub(
                "test-repos/branch-config",
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_COMMIT_3
        );

        service = new BranchYamlService(arcService, configurationService);
    }

    @Test
    void findBranchConfigWithAutocheckSection_simple() {
        var bundle = assertExisting("releases/simple");
        assertThat(bundle.getProblems()).isEmpty();

        Assertions.assertThat(bundle.getPath()).isEqualTo(Path.of("config/branches/releases/simple.yaml"));
        Assertions.assertThat(bundle.getRevision()).isEqualTo(TestData.TRUNK_COMMIT_3.getRevision());
    }

    @Test
    void findBranchConfigWithAutocheckSection_updated() {
        var bundle = assertExisting("releases/updated");
        assertThat(bundle.getProblems()).isEmpty();

        Assertions.assertThat(bundle.getPath()).isEqualTo(Path.of("config/branches/releases/updated.yaml"));
        Assertions.assertThat(bundle.getRevision()).isEqualTo(TestData.TRUNK_COMMIT_3.getRevision());
        Assertions.assertThat(bundle.getAutocheckSection().getDirs()).containsExactly("dir2");
    }

    @Test
    void findBranchConfigWithAutocheckSection_nearestWithAutocheckSection() {
        var bundle = assertExisting("releases/first/second/third");
        assertThat(bundle.getProblems()).isEmpty();

        Assertions.assertThat(bundle.getPath()).isEqualTo(Path.of("config/branches/releases/first/second.yaml"));
        Assertions.assertThat(bundle.getAutocheckSection().getDirs()).containsExactly("dir2");
    }

    @Test
    void findBranchConfigWithAutocheckSection_stopOnInvalid() {
        var bundle = assertExisting("releases/invalid/invalid");
        assertThat(bundle.getProblems()).isNotEmpty();

        Assertions.assertThat(bundle.getPath()).isEqualTo(Path.of("config/branches/releases/invalid/invalid.yaml"));
        Assertions.assertThat(bundle.isValid()).isFalse();
        Assertions.assertThatThrownBy(bundle::hasAutocheckSection)
                .isExactlyInstanceOf(IllegalStateException.class);
        Assertions.assertThatThrownBy(bundle::getAutocheckSection)
                .isExactlyInstanceOf(IllegalStateException.class);

    }

    private BranchConfigBundle assertExisting(String branchName) {
        var bundle = service.findBranchConfigWithAutocheckSection(ArcBranch.ofBranchName(branchName));
        assertThat(bundle).isPresent();
        return bundle.get();
    }

    @Test
    void getPossibleConfigPaths() {
        assertThat(service.getPossibleConfigPaths(ArcBranch.ofBranchName("releases/ci/main/42")))
                .containsExactly(
                        Path.of("config/branches/releases/ci/main/42.yaml"),
                        Path.of("config/branches/releases/ci/main.yaml"),
                        Path.of("config/branches/releases/ci.yaml")
                );

        assertThat(service.getPossibleConfigPaths(ArcBranch.ofBranchName("releases/ci/42")))
                .containsExactly(
                        Path.of("config/branches/releases/ci/42.yaml"),
                        Path.of("config/branches/releases/ci.yaml")
                );

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> service.getPossibleConfigPaths(ArcBranch.ofPullRequest(42L)));
    }

    @Test
    void findBranchConfigWithAutocheckSection_largeTestsNoConfig() {
        var bundle = assertExisting("releases/large-autostart");
        assertThat(bundle.getConfig()).isNotNull();
        assertThat(bundle.getDelegatedConfig()).isNull();
        assertThat(bundle.getProblems())
                .isEqualTo(List.of(ConfigProblem.crit("Unable to find valid config: ci/a.yaml")));
    }

    @Test
    void findBranchConfigWithAutocheckSection_largeTestsInvalidServices() {
        var config = Mockito.mock(ConfigBundle.class);
        Mockito.when(config.getValidAYamlConfig()).thenReturn(new AYamlConfig("ci", "test", null, null));
        Mockito.when(configurationService.findLastValidConfig(eq(Path.of("ci/a.yaml")), eq(ArcBranch.trunk())))
                .thenReturn(Optional.of(config));
        var bundle = assertExisting("releases/large-autostart");
        assertThat(bundle.getConfig()).isNotNull();
        assertThat(bundle.getDelegatedConfig()).isNull();
        assertThat(bundle.getProblems())
                .isEqualTo(List.of(ConfigProblem.crit("Branch service [simple] must be same as " +
                        "delegated service [ci]")));
    }

    @Test
    void findBranchConfigWithAutocheckSection_largeTests() {
        var config = Mockito.mock(ConfigBundle.class);
        Mockito.when(config.getValidAYamlConfig()).thenReturn(new AYamlConfig("simple", "test", null, null));
        Mockito.when(configurationService.findLastValidConfig(eq(Path.of("ci/a.yaml")), eq(ArcBranch.trunk())))
                .thenReturn(Optional.of(config));

        var bundle = assertExisting("releases/large-autostart");
        assertThat(bundle.getConfig()).isNotNull();
        assertThat(bundle.getDelegatedConfig()).isNotNull();
        assertThat(bundle.getProblems()).isEmpty();

        Assertions.assertThat(bundle.getPath()).isEqualTo(Path.of("config/branches/releases/large-autostart.yaml"));
        Assertions.assertThat(bundle.getRevision()).isEqualTo(TestData.TRUNK_COMMIT_3.getRevision());
    }
}
