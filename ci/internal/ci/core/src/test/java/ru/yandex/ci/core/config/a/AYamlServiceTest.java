package ru.yandex.ci.core.config.a;

import java.nio.file.Path;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.CoreYdbTestBase;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;

import static org.assertj.core.api.Assertions.assertThatThrownBy;

class AYamlServiceTest extends CoreYdbTestBase {

    @Autowired
    AYamlService aYamlService;

    @Autowired
    ArcService arcService;

    @Test
    void getConfig_whenAYamlNotFound() {
        var revision = ArcRevision.of("r1");
        var path = Path.of("ci/not-exists/a.yaml");
        assertThatThrownBy(() -> aYamlService.getConfig(path, revision))
                .isInstanceOf(AYamlNotFoundException.class);
    }

}
