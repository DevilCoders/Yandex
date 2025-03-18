package ru.yandex.ci.engine.autocheck;

import java.io.IOException;
import java.nio.file.Path;
import java.util.Optional;
import java.util.Set;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.engine.autocheck.model.BalancerConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.devtools.test.Paths;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.when;

@ContextConfiguration(classes = {
        FastCircuitTargetsAutoResolverTest.Config.class
})
public class BalancerConfigTest extends YdbCiTestBase {

    @MockBean
    private ArcService arcService;

    @Autowired
    private FastCircuitTargetsAutoResolver resolver;

    /**
     * Проверяет совместимость парсера с
     * <a href="https://a.yandex-team.ru/arc_vcs/autocheck/balancing_configs/autocheck-linux.json">
     *     актуальным конфигом балансировщика</a>.
     * <p>
     * Поломка этого теста означает, что сначала надо совместимым образом поправить код {@link BalancerConfig}.
     * </p>
     * <p color="red">Не работает в IDEA (из-за макроса ya.make DATA)</p>
     */
    @Test
    public void testRealConfig() throws IOException {
        Path configPath = Path.of(Paths.getSourcePath(FastCircuitTargetsAutoResolver.CONFIG_PATH));
        assertThat(configPath)
                .withFailMessage("Cannot find balancer config at expected path " + configPath)
                .exists();
        var parsedConfig = new ObjectMapper().readValue(configPath.toFile(), BalancerConfig.class);
        assertThat(parsedConfig.getPartitions()).isNotEmpty();
        parsedConfig.getPartitions().forEach((key, partition) -> assertThat(partition.size())
                .withFailMessage("Wrong number of partitions in section " + key + ": " + partition.size())
                .isEqualTo(key));
        assertThat(parsedConfig.getBiggestPartition()).isNotEmpty();
    }

    /**
     * Проверяет (обратную) совместимость парсера с прикопанным конфигом
     * Поломка этого теста означает, что сначала надо совместимым образом поправить и выкатить код CI,
     * и только после этого обновить прикопанный autocheck-linux.json
     */
    @Test
    public void testSavedConfig() {
        String savedConfigContent = ResourceUtils
                .textResource("autocheck/autocheck-linux-6cc1fd18af791421c12e45117c8dd7086fa2a95d.json");
        db.currentOrTx(() -> db.keyValue().setValue("AutocheckInfoCollector", "enabledForPercent", 100));
        ArcRevision headCommit = ArcRevision.of("head-commit-id");
        when(arcService.getLastRevisionInBranch(ArcBranch.trunk())).thenReturn(headCommit);
        when(arcService.getFileContent(anyString(), eq(headCommit))).thenReturn(Optional.of(savedConfigContent));

        assertThat(resolver.getFastTarget(Set.of(Path.of("util/tests/ut/file.txt")))).isNotEmpty();
        assertThat(resolver.getFastTarget(Set.of(Path.of("intranet/a.txt")))).isNotEmpty();
        assertThat(resolver.getFastTarget(Set.of(Path.of("/yweb/webscripts/learnquota/svm_light/file.txt"))))
                .isNotEmpty();
        assertThat(resolver.getFastTarget(Set.of(Path.of("/yweb/autoclassif/x")))).isNotEmpty();
        assertThat(resolver.getFastTarget(Set.of(Path.of("yweb/realtime/x")))).isNotEmpty();
        assertThat(resolver.getFastTarget(Set.of(Path.of("yweb/antispam/x")))).isNotEmpty();
    }
}
