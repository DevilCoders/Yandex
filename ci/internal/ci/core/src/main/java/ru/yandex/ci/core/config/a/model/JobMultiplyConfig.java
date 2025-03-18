package ru.yandex.ci.core.config.a.model;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class JobMultiplyConfig {

    // Both settings configured in a-yaml-ci-schema
    public static final int DEFAULT_JOBS = 20;
    public static final int MAX_JOBS = 99;

    @Nonnull
    @JsonProperty
    String by;

    @Nullable
    @JsonProperty
    String title;

    @Nullable
    @JsonProperty
    String description;

    @With
    @JsonProperty("max-jobs")
    @JsonAlias("maxJobs")
    int maxJobs;

    @JsonProperty("as-field") // Deprecated
    @JsonAlias("field")
    String field;

    public int getMaxJobs() { // Make sure we compatible with old configurations
        return maxJobs <= 0
                ? DEFAULT_JOBS
                :
                maxJobs > MAX_JOBS
                        ? MAX_JOBS
                        : maxJobs;
    }

    public static int index(int i) {
        return i + 1;
    }

    public static String getId(String jobId, int index) {
        return jobId + "-" + index;
    }
}
