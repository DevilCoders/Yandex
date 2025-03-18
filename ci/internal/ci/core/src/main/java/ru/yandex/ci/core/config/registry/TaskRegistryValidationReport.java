package ru.yandex.ci.core.config.registry;

import java.util.List;

import javax.annotation.Nullable;

import com.github.fge.jsonschema.core.report.ProcessingReport;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.config.SchemaReports;

@Value
@Builder
public class TaskRegistryValidationReport {
    @Nullable
    ProcessingReport schemaReport;

    public List<String> getErrorMessages() {
        return SchemaReports.getErrorMessages(schemaReport);
    }
}
