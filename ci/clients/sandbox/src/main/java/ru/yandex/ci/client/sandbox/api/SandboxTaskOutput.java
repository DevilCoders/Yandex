package ru.yandex.ci.client.sandbox.api;

import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.Value;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Builder
public class SandboxTaskOutput {
    private String type;
    private long id;
    private SandboxTaskStatus status;
    @Nullable
    private String author;
    @Nullable
    private String description;
    @Nullable
    private Map<String, Object> inputParameters;
    @Nullable
    private Map<String, Object> outputParameters;
    @Nullable
    private List<Report> reports;
    @Nullable
    private TaskExecution execution;
    @Nullable
    private List<String> tags;

    public SandboxTaskOutput(String type, long id, SandboxTaskStatus status) {
        this.type = type;
        this.id = id;
        this.status = status;
    }

    public SandboxTaskOutput(String type, long id, SandboxTaskStatus status,
                             @Nullable Map<String, Object> inputParameters,
                             @Nullable Map<String, Object> outputParameters,
                             @Nullable List<Report> reports) {
        this.type = type;
        this.id = id;
        this.status = status;
        this.inputParameters = inputParameters;
        this.outputParameters = outputParameters;
        this.reports = reports;
    }

    public long getId() {
        return id;
    }

    public SandboxTaskStatus getStatusEnum() {
        return status;
    }

    @SuppressWarnings("unchecked")
    public <T> Optional<T> getOutputParameter(String parameterName, Class<T> clazz) {
        Object value = outputParameters == null ? null : outputParameters.get(parameterName);
        if (value == null) {
            return Optional.empty();
        }
        if (clazz.isInstance(value)) {
            return Optional.of((T) value);
        }
        throw new RuntimeException(String.format(
                "There is no output parameter with name='%s' and type='%s'",
                parameterName,
                clazz.getName()
        ));
    }

    @Nonnull
    public Map<String, Object> getInputParameters() {
        return Collections.unmodifiableMap(Objects.requireNonNullElse(inputParameters, Map.of()));
    }

    @Nonnull
    public Map<String, Object> getOutputParameters() {
        return Collections.unmodifiableMap(Objects.requireNonNullElse(outputParameters, Map.of()));
    }

    @Nonnull
    public List<Report> getReports() {
        return Collections.unmodifiableList(Objects.requireNonNullElse(reports, List.of()));
    }

    @Nonnull
    public List<String> getTags() {
        return List.copyOf(Objects.requireNonNullElse(tags, List.of()));
    }

    @Value
    public static class Report {
        String url;
        String label;
        String title;
    }
}
