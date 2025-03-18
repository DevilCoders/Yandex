package ru.yandex.ci.core.config.branch;

import java.util.Set;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.github.fge.jsonschema.core.report.ProcessingReport;
import com.github.fge.jsonschema.main.JsonSchema;

import ru.yandex.ci.core.config.YamlParsers;
import ru.yandex.ci.core.config.YamlPreProcessor;
import ru.yandex.ci.core.config.branch.model.BranchYamlConfig;
import ru.yandex.ci.core.config.branch.validation.BranchYamlStaticValidator;
import ru.yandex.ci.core.config.branch.validation.BranchYamlValidationReport;

public class BranchYamlParser {
    private static final ObjectMapper MAPPER = YamlParsers.buildMapper();
    private static final JsonSchema SCHEMA = YamlParsers.buildValidationSchema(
            MAPPER, "ci-config-schema/branch-yaml.yaml"
    );

    private BranchYamlParser() {
    }

    public static BranchYamlValidationReport parseAndValidate(String yamlSource) throws JsonProcessingException,
            ProcessingException {
        var yaml = YamlPreProcessor.preprocess(yamlSource);

        JsonNode configModel = MAPPER.readTree(yaml);
        if (configModel.isMissingNode()) {
            configModel = MAPPER.createObjectNode();
        }
        ProcessingReport report = SCHEMA.validate(configModel);
        if (!report.isSuccess()) {
            return new BranchYamlValidationReport(report, Set.of(), null);
        }
        BranchYamlConfig config = MAPPER.readValue(yaml, BranchYamlConfig.class);
        var staticErrors = new BranchYamlStaticValidator(config).validate();
        return new BranchYamlValidationReport(
                report,
                staticErrors,
                staticErrors.isEmpty() ? config : null);
    }

}
