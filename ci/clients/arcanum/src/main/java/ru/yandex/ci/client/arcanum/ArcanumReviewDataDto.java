package ru.yandex.ci.client.arcanum;

import java.util.List;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class ArcanumReviewDataDto {
    long id;
    String summary;
    Person author;
    String commitDescription;
    String description;
    Vcs vcs;
    List<ArcanumMergeRequirementDto> checks;
    DiffSet activeDiffSet;
    List<DiffSet> diffSets;
    List<Person> approvers;
    CiCheckSettings ciSettings;

    public List<ArcanumMergeRequirementDto> getChecks() {
        return Objects.requireNonNullElse(checks, List.of());
    }

    public String getSummary() {
        return Objects.requireNonNullElse(summary, "");
    }

    @Value
    public static class Vcs {
        String type;
        String fromBranch;
        String toBranch;
    }

    @Value
    public static class DiffSet {
        long id;
        String gsid;
        String description;
        String status;
        String patchUrl;
        ArcBranchHeads arcBranchHeads;
        Zipatch zipatch;

        @SuppressWarnings("ParameterMissingNullable")
        public static DiffSet of(long id) {
            return new DiffSet(id, null, null, null, null, null, null);
        }
    }

    @Value
    public static class ArcBranchHeads {
        @JsonProperty("from_id")
        String featureRevision;
        @JsonProperty("to_id")
        String upstreamRevision;
        @JsonProperty("merge_id")
        String mergeRevision;
    }

    @Value
    public static class Person {
        String name;
    }

    @Value
    public static class Zipatch {
        private static final String ZIPATCH_PREFIX = "zipatch:";

        String url;
        int svnBaseRevision;
        String svnBranch;

        public String getUrlWithPrefix() {
            return url.startsWith(ZIPATCH_PREFIX) ? url : ZIPATCH_PREFIX + url;
        }
    }

    @Value
    public static class CiCheckSettings {
        @Nullable
        FastCircuit fastCircuit;
        @Nullable
        CheckFastMode checkFastMode;

        public boolean isValidForFastCircuit() {
            return Optional.ofNullable(fastCircuit)
                    .map(FastCircuit::getFastTargets)
                    .map(it -> !it.isEmpty())
                    .orElse(false);
        }
    }

    @Value
    public static class FastCircuit {
        List<String> fastTargets;
        Boolean onlyFastCircuit;
    }

    public enum CheckFastMode {
        @JsonProperty("auto")
        AUTO,
        @JsonProperty("parallel")
        PARALLEL,
        @JsonProperty("parallel-no-fail-fast")
        PARALLEL_NO_FAIL_FAST,
        @JsonProperty("disabled")
        DISABLED,
        @JsonProperty("fast-only")
        FAST_ONLY,
        @JsonProperty("sequential")
        SEQUENTIAL
    }

}
