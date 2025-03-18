package ru.yandex.ci.core.config.branch;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import com.github.fge.jsonschema.core.report.ProcessingMessage;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.config.a.model.LargeAutostartConfig;
import ru.yandex.ci.core.config.branch.model.BranchAutocheckConfig;
import ru.yandex.ci.core.config.branch.model.BranchCiConfig;
import ru.yandex.ci.core.config.branch.model.BranchYamlConfig;
import ru.yandex.ci.core.config.branch.validation.BranchYamlValidationReport;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

public class BranchYamlParserTest {
    private static String readResource(String resource) {
        return TestUtils.textResource(resource);
    }

    private static BranchYamlConfig simpleBranchYaml(LargeAutostartConfig... largeAutostartConfigs) {
        BranchAutocheckConfig autocheckConfig = BranchAutocheckConfig.builder()
                .pool("autocheck/precommits/devtools")
                .dirs(List.of("ci/registry", "testenv/jobs"))
                .strong(true)
                .largeAutostart(List.of(largeAutostartConfigs))
                .build();
        BranchCiConfig ciConfig = new BranchCiConfig("ci/a.yaml", autocheckConfig);
        return new BranchYamlConfig("yt", ciConfig);
    }

    @Test
    void referenceConfigTest() throws Exception {
        String yaml = readResource("branch-yaml/simple.yaml");
        BranchYamlValidationReport validationReport = BranchYamlParser.parseAndValidate(yaml);
        assertThat(validationReport.isSuccess()).isTrue();

        assertThat(validationReport.getConfig()).
                isEqualTo(simpleBranchYaml(
                        new LargeAutostartConfig("ci/demo-projects/large-tests"),
                        new LargeAutostartConfig("ci/demo-projects/large-tests/test2",
                                List.of("default-linux-x86_64-release", "default-linux-x86_64-release-musl"))
                ));
    }

    @Test
    void largeSimple1Test() throws Exception {
        String yaml = readResource("branch-yaml/simple-large-simple-1.yaml");
        BranchYamlValidationReport validationReport = BranchYamlParser.parseAndValidate(yaml);
        assertThat(validationReport.isSuccess()).isTrue();
        assertThat(validationReport.getConfig())
                .isEqualTo(simpleBranchYaml(
                        new LargeAutostartConfig("ci/demo-projects/large-tests")
                ));
    }

    @Test
    void largeSimple2Test() throws Exception {
        String yaml = readResource("branch-yaml/simple-large-simple-2.yaml");
        BranchYamlValidationReport validationReport = BranchYamlParser.parseAndValidate(yaml);
        assertThat(validationReport.isSuccess()).isTrue();
        assertThat(validationReport.getConfig())
                .isEqualTo(simpleBranchYaml(
                        new LargeAutostartConfig("ci/demo-projects/large-tests"),
                        new LargeAutostartConfig("ci/demo-projects/large-tests/test2")
                ));
    }

    @ParameterizedTest
    @MethodSource
    void validatorTest(String filename, List<String> errorMessages) throws Exception {
        String yaml = readResource("branch-yaml/" + filename + ".yaml");
        BranchYamlValidationReport validationReport = BranchYamlParser.parseAndValidate(yaml);

        var actualMessages = new ArrayList<String>();
        var processingReport = validationReport.getSchemaReport();
        if (processingReport != null) {
            StreamSupport.stream(processingReport.spliterator(), false)
                    .map(ProcessingMessage::getMessage)
                    .forEach(actualMessages::add);
        }
        actualMessages.addAll(validationReport.getStaticErrors());
        assertThat(actualMessages).isEqualTo(errorMessages);
    }

    static Stream<Arguments> validatorTest() {
        return Stream.of(
                Arguments.of("no-services", List.of("object has missing required properties ([\"service\"])")),
                Arguments.of("single-dir", List.of()),
                Arguments.of("single-service", List.of()),
                Arguments.of(
                        "no-dirs",
                        List.of("instance type (null) does not match any allowed primitive type " +
                                "(allowed: [\"object\"])")
                ),
                Arguments.of("no-delegated", List.of()),
                Arguments.of("no-strong-mode", List.of()),
                Arguments.of("not-ci-config", List.of()),
                Arguments.of("no-delegated-with-autostart",
                        List.of("Delegated config is required when large autostart settings are provided")),
                Arguments.of("many-delegates",
                        List.of("instance type (array) does not match any allowed primitive type " +
                                "(allowed: [\"string\"])"))
        );
    }

}
