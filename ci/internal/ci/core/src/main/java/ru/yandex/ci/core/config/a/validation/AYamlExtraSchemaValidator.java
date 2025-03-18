package ru.yandex.ci.core.config.a.validation;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.JsonNodeFactory;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.github.fge.jsonschema.core.report.ListProcessingReport;
import com.github.fge.jsonschema.core.report.ProcessingMessage;
import com.github.fge.jsonschema.core.report.ProcessingReport;
import lombok.AllArgsConstructor;

@AllArgsConstructor
public class AYamlExtraSchemaValidator {
    private static final Map<String, List<String>> DEPENDENT_PROPERTIES = Map.of(
            "/ci/autocheck/large-autostart", List.of("/ci/secret", "/ci/runtime")
    );

    @Nonnull
    private final JsonNode config;

    public ProcessingReport validate() throws ProcessingException {
        ProcessingReport dependenciesReport = new ListProcessingReport();

        for (Map.Entry<String, List<String>> dependency: DEPENDENT_PROPERTIES.entrySet()) {
            JsonNode pointerNode = config.at(dependency.getKey());

            if (pointerNode.isMissingNode()) {
                continue;
            }

            List<String> missingPointers = new ArrayList<>();

            for (String requiredPointer : dependency.getValue()) {
                if (config.at(requiredPointer).isMissingNode()) {
                    missingPointers.add(requiredPointer);
                }
            }

            if (!missingPointers.isEmpty()) {
                dependenciesReport.error(
                        createProcessingMessage(dependency, missingPointers)
                );
            }
        }

        return dependenciesReport;
    }

    private ProcessingMessage createProcessingMessage(Map.Entry<String, List<String>> dependency,
                                                      List<String> missingPointers) {
        String pointer = dependency.getKey();
        int splitPos = pointer.lastIndexOf("/");

        return new ProcessingMessage().setMessage(
                String.format(
                        "property \"%s\" of object has missing property dependencies " +
                                "(requires %s; missing: %s)",
                        pointer.substring(splitPos + 1),
                        dependency.getValue(),
                        missingPointers
                )).put("required", dependency.getValue())
                .put("missing", missingPointers)
                .put(
                        "instance",
                        JsonNodeFactory.instance.objectNode().put("pointer", pointer.substring(0, splitPos))
                );
    }
}
