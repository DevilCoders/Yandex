package ru.yandex.ci.core.config.a.model;

import java.util.LinkedHashMap;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Setter;
import lombok.ToString;
import lombok.Value;
import lombok.With;
import lombok.experimental.NonFinal;

import ru.yandex.ci.core.config.ConfigIdEntry;
import ru.yandex.ci.core.config.ConfigUtils;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;
import ru.yandex.ci.util.jackson.parse.ParseInfo;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(buildMethodName = "buildInternal", toBuilder = true)
@JsonDeserialize(builder = FlowConfig.Builder.class)
public class FlowConfig implements ConfigIdEntry<FlowConfig>, HasParseInfo {

    @JsonIgnore
    @With(onMethod_ = @Override)
    @Getter(onMethod_ = @Override)
    String id;

    @JsonProperty
    String title;

    @Nullable
    @JsonProperty
    String description;

    @Nullable
    @JsonProperty("show-in-actions")
    @JsonFormat
    Boolean showInActions;

    @With
    @JsonProperty
    LinkedHashMap<String, JobConfig> jobs;

    @JsonProperty("cleanup-jobs")
    LinkedHashMap<String, JobConfig> cleanupJobs;

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @Getter(onMethod_ = @Override)
    @Setter(onMethod_ = @Override)
    @NonFinal
    @JsonIgnore
    transient ParseInfo parseInfo;

    public String getTitle() {
        return Objects.requireNonNullElse(title, id);
    }

    public Optional<JobConfig> findJob(String jobId) {
        return Optional.ofNullable(jobs.get(jobId));
    }

    public JobConfig getJob(String jobId) {
        return findJob(jobId).orElseThrow(() ->
                new IllegalArgumentException("Job not found " + jobId));
    }

    public static class Builder {
        {
            this.jobs = new LinkedHashMap<>();
            this.cleanupJobs = new LinkedHashMap<>();
        }

        public Builder job(JobConfig... jobs) {
            for (var job : jobs) {
                this.jobs.put(job.getId(), job);
            }
            return this;
        }

        public Builder cleanupJob(JobConfig... cleanupJobs) {
            for (var cleanupJob : cleanupJobs) {
                this.cleanupJobs.put(cleanupJob.getId(), cleanupJob);
            }
            return this;
        }

        public FlowConfig build() {
            jobs(ConfigUtils.update(jobs));
            cleanupJobs(ConfigUtils.update(cleanupJobs));
            return buildInternal();
        }
    }
}
