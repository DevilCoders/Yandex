package ru.yandex.ci.core.config.a.validation;

import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.github.fge.jsonschema.core.report.ProcessingReport;
import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Singular;
import lombok.ToString;
import lombok.Value;

import ru.yandex.ci.core.config.SchemaReports;
import ru.yandex.ci.core.config.a.model.AYamlConfig;

@ToString(doNotUseGetters = true)
@EqualsAndHashCode(doNotUseGetters = true)
@Value
@Builder
public class ValidationReport {
    @Nullable
    ProcessingReport schemaReport;

    @Singular
    Set<String> staticErrors;

    @Nonnull
    FlowReport flowReport;

    @Nullable
    AYamlConfig config;

    private ValidationReport(
            @Nullable ProcessingReport schemaReport,
            @Nonnull Set<String> staticErrors,
            @Nullable FlowReport flowReport,
            @Nullable AYamlConfig config
    ) {
        this.schemaReport = schemaReport;
        this.staticErrors = staticErrors;
        this.flowReport = Objects.requireNonNullElse(flowReport, FlowReport.EMPTY);

        Preconditions.checkArgument(
                isSuccess() == (config != null),
                "config = %s, but success = %s",
                config,
                isSuccess()
        );
        this.config = config;
    }


    public boolean isSuccess() {
        return (schemaReport == null || schemaReport.isSuccess())
                && staticErrors.isEmpty()
                && flowReport.isSuccess();
    }

    public AYamlConfig getConfig() {
        if (config == null) {
            throw new IllegalStateException("Expect valid configuration but configuration has errors: " + this);
        }
        return config;
    }

    public List<String> getSchemaReportMessages() {
        return SchemaReports.getErrorMessages(schemaReport);
    }


    @Value
    public static class FlowReport {

        private static final FlowReport EMPTY = new FlowReport(null);

        Map<String, Set<String>> jobErrors;

        public FlowReport(@Nullable Map<String, Set<String>> jobErrors) {
            this.jobErrors = jobErrors != null ? Map.copyOf(jobErrors) : Map.of();
        }

        public boolean isSuccess() {
            return jobErrors.isEmpty();
        }
    }

}
