package ru.yandex.ci.engine.autocheck.config;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Optional;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.devtools.test.Paths;

import static org.assertj.core.api.Assertions.assertThat;

class AutocheckYamlServiceTest {

    /**
     * !!! Не работает в IDEA (из-за макроса DATA) !!!
     * Проверяет совместимость парсера с актуальным конфигом автосборки
     * https://a.yandex-team.ru/arc/trunk/arcadia/autocheck/autocheck.yaml
     * Поломка этого теста означает, что сначала надо совместимым образом поправить код CI
     */
    @Test
    void testRealConfig() throws IOException {
        Path configPath = Path.of(Paths.getSourcePath(AutocheckYamlService.AUTOCHECK_YAML_PATH.toString()));
        Assertions.assertThat(new File(configPath.toString())).exists();
        String configContent = Files.readString(configPath);

        ArcService arcService = Mockito.mock(ArcService.class);
        AutocheckYamlService service = new AutocheckYamlService(arcService);
        Mockito.when(arcService.getFileContent(AutocheckYamlService.AUTOCHECK_YAML_PATH, TestData.TRUNK_R4))
                .thenReturn(Optional.of(configContent));

        AutocheckYamlConfig config = service.getConfig(TestData.TRUNK_R4);
        assertThat(config.getConfigurations()).hasSizeGreaterThanOrEqualTo(1);
    }

    /**
     * Проверяет (обратную) совместимость парсера с прикопанным конфигом
     * Поломка этого теста означает, что сначала надо совместимым образом поправить и выкатить код CI,
     * и только после этого обновить прикопанный autocheck.yaml
     */
    @Test
    void testSavedConfig() throws Exception {

        String savedConfigContent = ResourceUtils.textResource("autocheck/autocheck-saved-on-r8488422.yaml");

        ArcService arcService = Mockito.mock(ArcService.class);
        AutocheckYamlService service = new AutocheckYamlService(arcService);

        Mockito.when(arcService.getLastRevisionInBranch(ArcBranch.trunk()))
                .thenReturn(TestData.TRUNK_COMMIT_4.getRevision());

        Mockito.when(arcService.getLastCommit(
                AutocheckYamlService.AUTOCHECK_YAML_PATH, TestData.TRUNK_COMMIT_4.getRevision()
        )).thenReturn(Optional.of(TestData.TRUNK_COMMIT_2));

        Mockito.when(arcService.getFileContent(
                AutocheckYamlService.AUTOCHECK_YAML_PATH, TestData.TRUNK_COMMIT_2.getRevision()
        )).thenReturn(Optional.of(savedConfigContent));

        AutocheckYamlService.ConfigBundle configBundle = service.getLastTrunkConfig();
        assertThat(configBundle.getRevision()).isEqualTo(TestData.TRUNK_COMMIT_2.getRevision());
        AutocheckYamlConfig actualConfig = configBundle.getConfig();

        assertThat(actualConfig.getConfigurations()).hasSize(5);
        assertThat(
                actualConfig.getConfigurations().stream().filter(AutocheckConfigurationConfig::isEnabled)
        ).hasSize(5);
        assertThat(actualConfig.getConfigurations().get(0)).isEqualTo(savedLinuxJob());
    }

    private AutocheckConfigurationConfig savedLinuxJob() throws Exception {

        return new AutocheckConfigurationConfig(
                "linux",
                new AutocheckPartitionsConfig(6),
                true
        );

    }
}
