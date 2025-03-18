package ru.yandex.ci.core.config.registry;

import com.fasterxml.jackson.annotation.JsonSetter;
import com.fasterxml.jackson.annotation.Nulls;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.github.fge.jsonschema.core.report.ProcessingReport;
import com.github.fge.jsonschema.main.JsonSchema;

import ru.yandex.ci.core.config.YamlParsers;
import ru.yandex.ci.core.config.YamlParsers.SchemaMapping;
import ru.yandex.ci.core.config.YamlPreProcessor;

public class TaskConfigYamlParser {
    // TODO: Make standard
    private static final ObjectMapper MAPPER = YamlParsers.buildMapper(null)
            .setDefaultSetterInfo(JsonSetter.Value.forContentNulls(Nulls.AS_EMPTY));

    private static final JsonSchema SCHEMA = YamlParsers.buildValidationSchema(
            MAPPER,
            "ci-config-schema/registry-task.yaml",
            SchemaMapping.of("schema://ci/schemas/a-yaml", "ci-config-schema/a-yaml.yaml")
    );

    private TaskConfigYamlParser() {
    }

    public static TaskConfig parse(String yamlSource) throws JsonProcessingException {
        return parse(yamlSource, false);
    }

    public static TaskConfig parse(String yamlSource, boolean skipValidation) throws JsonProcessingException {
        try {
            var yaml = YamlPreProcessor.preprocess(yamlSource);
            var rootNode = MAPPER.readTree(yaml);
            if (!skipValidation) {
                ProcessingReport schemaReport = SCHEMA.validate(rootNode);
                if (!schemaReport.isSuccess()) {
                    throw new TaskRegistryValidationException(
                            TaskRegistryValidationReport.builder()
                                    .schemaReport(schemaReport)
                                    .build()
                    );
                }
            }

            return MAPPER.readValue(yaml, TaskConfig.class);

        } catch (ProcessingException e) {
            throw new TaskRegistryException(e.getMessage());
        }
    }
}
