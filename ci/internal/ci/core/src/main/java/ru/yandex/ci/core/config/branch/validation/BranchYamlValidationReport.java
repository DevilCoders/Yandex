package ru.yandex.ci.core.config.branch.validation;

import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.github.fge.jsonschema.core.report.ProcessingReport;
import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.core.config.SchemaReports;
import ru.yandex.ci.core.config.branch.model.BranchYamlConfig;

@Value
public class BranchYamlValidationReport {
    @Nullable
    ProcessingReport schemaReport;

    @Nonnull
    Set<String> staticErrors;

    @Nullable
    BranchYamlConfig config;

    public BranchYamlValidationReport(
            @Nullable ProcessingReport schemaReport,
            @Nonnull Set<String> staticErrors,
            @Nullable BranchYamlConfig config
    ) {
        this.schemaReport = schemaReport;
        this.staticErrors = staticErrors;
        Preconditions.checkArgument(
                isSuccess() == (config != null),
                "config = %s, but success = %s",
                config,
                isSuccess()
        );
        this.config = config;
    }


    public boolean isSuccess() {
        return (schemaReport == null || schemaReport.isSuccess()) && staticErrors.isEmpty();
    }

    public List<String> getSchemaReportMessages() {
        return SchemaReports.getErrorMessages(schemaReport);
    }

}
