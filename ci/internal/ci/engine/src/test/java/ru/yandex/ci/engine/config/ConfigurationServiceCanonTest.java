package ru.yandex.ci.engine.config;

import java.nio.file.Path;

import org.junit.jupiter.api.Test;
import org.springframework.test.context.ActiveProfiles;

import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.engine.EngineTestBase;

import static org.assertj.core.api.Assertions.assertThat;

@ActiveProfiles(ArcClientTestConfig.CANON_ARC_PROFILE)
public class ConfigurationServiceCanonTest extends EngineTestBase {

    @Test
    void getMobileConfig() {
        abcServiceStub.addService(Abc.of(111, "mail-android", "mail-android"));

        var config = configurationService.getOrCreateConfig(Path.of("mobile/mail/android/mail-app/a.yaml"),
                OrderedArcRevision.fromHash("0425f2e546482a492a72255c754a076a34827826", ArcBranch.trunk(), 7906361, 0)
        ).orElseThrow();

        assertThat(config.getStatus()).isEqualTo(ConfigStatus.NOT_CI);
    }
}
