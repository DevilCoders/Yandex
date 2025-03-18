package ru.yandex.ci.core.config;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.StreamSupport;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.JsonNode;
import com.github.fge.jsonschema.core.report.ProcessingMessage;
import com.github.fge.jsonschema.core.report.ProcessingReport;

public class SchemaReports {

    private SchemaReports() {

    }

    public static List<String> getErrorMessages(@Nullable ProcessingReport schemaReport) {
        if (schemaReport == null) {
            return List.of();
        }
        return StreamSupport.stream(schemaReport.spliterator(), false)
                .map(ProcessingMessage::asJson)
                .map(JsonNode::toPrettyString)
                .collect(Collectors.toList());
    }
}
