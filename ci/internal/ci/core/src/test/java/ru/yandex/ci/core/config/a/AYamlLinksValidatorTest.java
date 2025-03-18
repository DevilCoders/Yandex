package ru.yandex.ci.core.config.a;

import java.util.Set;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

public class AYamlLinksValidatorTest {

    @Test
    void testValidateJobsCycles() throws Exception {
        var result = AYamlParser.parseAndValidate(TestUtils.textResource("ayaml/invalid-schema/jobs-cycles.yaml"));
        assertThat(result.isSuccess()).isFalse();

        Set<String> errorMessages = result.getStaticErrors();
        assertThat(errorMessages)
                .containsExactlyInAnyOrder(
                        "Jobs cycle found in /ci/flows/flow_4/jobs: 2 -> 3 -> 4 -> 2.",
                        "Jobs cycle found in /ci/flows/flow_7/jobs: 1 -> 3 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_4/jobs: 1 -> 5 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_5/jobs: 1 -> 2 -> 4 -> 5 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_5/jobs: 1 -> 2 -> 3 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_6/jobs: 1 -> 2 -> 3 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_7/jobs: 1 -> 2 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_2/jobs: 1 -> 2 -> 3 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_1/jobs: 1 -> 2 -> 1.",
                        "Jobs cycle found in /ci/flows/flow_3/jobs: 2 -> 3 -> 4 -> 2.",
                        "Jobs cycle found in /ci/flows/flow_6/jobs: 2 -> 4 -> 5 -> 2."
                );
    }

    @Test
    void testValidateFlows() throws Exception {
        var result = AYamlParser.parseAndValidate(TestUtils.textResource("ayaml/invalid-schema/invalid-flows.yaml"));
        assertThat(result.isSuccess()).isFalse();

        Set<String> expected = Set.of(
                "flow 'invalid-sawmill' in /ci/triggers/2/flow not found in /ci/flows",
                "flow 'invalid-sawmill' in /ci/triggers/3/flow not found in /ci/flows",
                "flow 'invalid-my-app-release' in /ci/releases/invalid-my-app/flow not found in /ci/flows"
        );

        Set<String> errorMessages = result.getStaticErrors();
        Assertions.assertEquals(expected, errorMessages);
    }

    @Test
    void testValidateJobs() throws Exception {
        var result = AYamlParser.parseAndValidate(TestUtils.textResource("ayaml/invalid-schema/invalid-jobs.yaml"));
        assertThat(result.isSuccess()).isFalse();

        Set<String> expected = Set.of(
                "Job 'invalid-job-3' in /ci/flows/flow_2/jobs/3/needs not found in /ci/flows/flow_2/jobs",
                "Job 'invalid-job-2' in /ci/flows/flow_2/jobs/3/needs not found in /ci/flows/flow_2/jobs",
                "Job 'invalid-job-1' in /ci/flows/flow_1/jobs/1/needs not found in /ci/flows/flow_1/jobs"
        );

        Set<String> errorMessages = result.getStaticErrors();
        Assertions.assertEquals(expected, errorMessages);
    }
}
