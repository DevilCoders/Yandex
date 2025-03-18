package ru.yandex.ci.engine.config;

import java.nio.file.Path;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;

import static org.assertj.core.api.Assertions.assertThat;

@Import(DeleteCiSectionConfigurationTest.Config.class)
public class DeleteCiSectionConfigurationTest extends EngineTestBase {

    @BeforeEach
    void setUp() {
        mockYavAny();
        mockSandboxDelegationAny();
        mockValidationSuccessful();
    }

    @Test
    void deleteCiSection() {
        var configPath = Path.of("a.yaml");
        discovery(TestData.TRUNK_COMMIT_2);
        delegateToken(configPath);

        var configStateWithCi = db.currentOrReadOnly(() -> db.configStates().get(configPath));
        assertThat(configStateWithCi.getStatus()).isEqualTo(ConfigState.Status.OK);

        discovery(TestData.TRUNK_COMMIT_3);
        delegateToken(configPath);

        var configStateWithoutCi = db.currentOrReadOnly(() -> db.configStates().get(configPath));
        assertThat(configStateWithoutCi.getStatus()).isEqualTo(ConfigState.Status.NOT_CI);
    }

    @TestConfiguration
    public static class Config {
        @Bean
        public ArcService arcService() {
            return new ArcServiceStub("test-repos/delete-ci-section",
                    TestData.TRUNK_COMMIT_2,
                    TestData.TRUNK_COMMIT_3
            );
        }
    }

}

