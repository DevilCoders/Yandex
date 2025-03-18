package ru.yandex.ci.core.config.a;

import java.util.Set;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.github.fge.jsonschema.core.report.ProcessingReport;
import com.github.fge.jsonschema.main.JsonSchema;
import com.google.common.base.Preconditions;
import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.YamlParsers;
import ru.yandex.ci.core.config.YamlPreProcessor;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.validation.AYamlExtraSchemaValidator;
import ru.yandex.ci.core.config.a.validation.AYamlStaticValidator;
import ru.yandex.ci.core.config.a.validation.ValidationReport;

@Slf4j
public class AYamlParser {

    private static final ObjectMapper MAPPER = YamlParsers.buildMapper();
    private static final JsonSchema SCHEMA = YamlParsers.buildValidationSchema(
            MAPPER, "ci-config-schema/a-yaml.yaml"
    );

    private AYamlParser() {
    }

    /**
     * Get parsed and validated configuration.
     *
     * @param yamlSource YML file content
     * @return parsed and validated config
     * @throws JsonProcessingException jackson parser errors
     * @throws ProcessingException     YML schema validation errors
     */
    public static ValidationReport parseAndValidate(@Nonnull String yamlSource)
            throws JsonProcessingException, ProcessingException {
        return parseAndValidate(yamlSource, null);
    }

    /**
     * Get parsed and validated configuration.
     *
     * @param yamlSource YML file content
     * @param commit current yaml revision
     * @return parsed and validated config
     * @throws JsonProcessingException jackson parser errors
     * @throws ProcessingException     YML schema validation errors
     */
    public static ValidationReport parseAndValidate(@Nonnull String yamlSource, @Nullable ArcCommit commit)
            throws JsonProcessingException, ProcessingException {
        var stopWatch = Stopwatch.createStarted();
        try {
            var yaml = YamlPreProcessor.preprocess(yamlSource);

            var rootNode = MAPPER.readTree(yaml);
            JsonNode configModel;
            if (rootNode.isMissingNode()) {
                // пустой yaml трактуем как пустой словарь
                configModel = MAPPER.createObjectNode();
            } else {
                configModel = rootNode;
            }
            ProcessingReport report = SCHEMA.validate(configModel);
            report.mergeWith(new AYamlExtraSchemaValidator(configModel).validate());

            var builder = ValidationReport.builder().schemaReport(report);
            if (report.isSuccess()) {
                var config = MAPPER.readValue(yaml, AYamlConfig.class);
                if (config.getCi() != null) {
                    config = config.withCi(AYamlPostProcessor.postProcessCi(config.getCi()));
                }
                Set<String> errors = new AYamlStaticValidator(config, commit).validate();
                builder.staticErrors(errors);
                if (errors.isEmpty()) {
                    builder.config(config);
                }

                // Нужно для теста, который проверяет брошенные исключения, не связанные с разбором YAML
                // См. ConfigParseServiceTest#internalValidationException
                Preconditions.checkState(
                        !config.getService().equals("throw-me-exception"),
                        "Just for test"
                );

            }
            return builder.build();
        } finally {
            log.info("Configuration loaded within {} msec", stopWatch.stop().elapsed(TimeUnit.MILLISECONDS));
            stopWatch.start();
        }
    }

    public static String toYaml(AYamlConfig config) throws JsonProcessingException {
        return MAPPER.writeValueAsString(config);
    }

    public static ObjectMapper getMapper() {
        return MAPPER;
    }
}
