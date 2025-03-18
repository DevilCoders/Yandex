package ru.yandex.ci.core.config.a.validation;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import org.apache.commons.lang3.StringUtils;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

class AYamlStageValidatorTest {

    private static final String A = "A";
    private static final String B = "B";
    private static final String C = "C";
    private static final String D = "D";
    private static final String E = "E";

    private static final String FIRST = "first";
    private static final String SECOND = "second";
    private static final String THIRD = "third";
    private static final String FOURTH = "fourth";


    /*
     *         ┌─·C·─┐
     *  A·─·B·─┤     ├─·E
     *         └─·D·─┘
     */

    @Test
    void validForSkippedStage() throws JsonProcessingException, ProcessingException {
        var result = validateTemplate(Map.of(
                A, FIRST,
                B, THIRD));

        assertThat(result.isSuccess()).isTrue();
    }

    @Test
    void validForDiamondFlow() throws JsonProcessingException, ProcessingException {
        var result = validateTemplate(Map.of(
                A, FIRST,
                B, SECOND,
                E, THIRD));

        assertThat(result.isSuccess()).isTrue();
    }

    @Test
    void detectsParallelStages() throws JsonProcessingException, ProcessingException {
        var result = validateTemplate(Map.of(
                A, FIRST,
                C, SECOND,
                D, THIRD,
                E, FOURTH));

        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStaticErrors()).isEqualTo(Set.of(
                "Parallel stages detected in flow /ci/flows/flow-1"));
    }

    @Test
    void detectsMultipleEntriesStages() throws JsonProcessingException, ProcessingException {
        var result = validateTemplate(Map.of(
                A, FIRST,
                C, SECOND,
                D, SECOND,
                E, THIRD));

        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStaticErrors()).isEqualTo(Set.of(
                "Stage second must have one entry point. Flow /ci/flows/flow-1"));
    }

    @Test
    void detectsJobWithoutStage() throws JsonProcessingException, ProcessingException {
        var result = validateTemplate(Map.of(
                B, SECOND));

        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStaticErrors()).isEqualTo(Set.of(
                "All jobs in flow /ci/flows/flow-1 must have stages, invalid jobs: [A]"));
    }

    @Test
    void detectsWrongStageOrder() throws JsonProcessingException, ProcessingException {
        var result = validateTemplate(Map.of(
                A, SECOND,
                B, FIRST));

        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStaticErrors()).isEqualTo(Set.of(
                "Downstream cannot have earlier stage than upstream," +
                        " downstream job = B at /ci/flows/flow-1/jobs/B, downstream stage = first," +
                        " upstream job = A at /ci/flows/flow-1/jobs/A, upstream stage = second"));
    }

    @Test
    void detectsJobWithoutDownstreamInNotLastStage() throws JsonProcessingException, ProcessingException {
        /*
         *         ┌─·C
         *  A·─·B·─┤
         *         └─·D
         */
        var result = AYamlParser.parseAndValidate(TestUtils.textResource("ayaml/stages/template-two-ends.yaml"));
        assertThat(result.isSuccess()).isFalse();
        assertThat(result.getStaticErrors()).isEqualTo(Set.of(
                "Staged flow must have jobs without downstream only on last stage, " +
                        "got first for jobs C; second for jobs D, flow /ci/flows/flow-1"));
    }

    private ValidationReport validateTemplate(Map<String, String> stageMap) throws JsonProcessingException,
            ProcessingException {
        var resource = TestUtils.textResource("ayaml/stages/template.yaml");
        return AYamlParser.parseAndValidate(remap(resource, stageMap));
    }

    private static String remap(String text, Map<String, String> stageMap) {
        List<String> keys = new ArrayList<>();
        List<String> values = new ArrayList<>();

        for (Map.Entry<String, String> e : stageMap.entrySet()) {
            keys.add(e.getKey());
            values.add("stage: " + e.getValue());
        }

        for (var key : Set.of(A, B, C, D, E)) {
            if (!stageMap.containsKey(key)) {
                keys.add(key);
                values.add("");
            }
        }

        return StringUtils.replaceEach(text,
                keys.stream().map(k -> "${" + k + ".stage}").toArray(String[]::new),
                values.toArray(new String[0]));
    }
}
