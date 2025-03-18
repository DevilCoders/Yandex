package ru.yandex.ci.core.config.a.model;

import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Setter;
import lombok.Singular;
import lombok.ToString;
import lombok.Value;
import lombok.With;
import lombok.experimental.NonFinal;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;
import ru.yandex.ci.util.jackson.parse.ParseInfo;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Persisted
@Value
@Builder
@JsonDeserialize(builder = TriggerConfig.Builder.class)
public class TriggerConfig implements HasParseInfo, HasFlowRef {

    @Nonnull
    @JsonProperty
    On on;

    @Nullable
    @JsonProperty
    @With
    String flow;

    @Nonnull
    @JsonProperty
    @Singular
    List<FilterConfig> filters;

    @Nullable
    @JsonProperty("into")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<Into> targetBranches;

    @Nullable
    @JsonProperty("required")
    Boolean required;

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @Getter(onMethod_ = @Override)
    @Setter(onMethod_ = @Override)
    @NonFinal
    @JsonIgnore
    transient ParseInfo parseInfo;

    @JsonIgnore
    public Set<ArcBranch.Type> getTargetBranchesOrDefault() {
        return targetBranches != null
                ? targetBranches.stream().map(Into::getAcceptedBranchType).collect(Collectors.toSet())
                : Set.of(Into.TRUNK.getAcceptedBranchType());
    }

    @JsonIgnore
    public Boolean getRequired() {
        if (on == On.PR && required == null) {
            return true;
        }
        return required;
    }

    // NOTE: Used only during validation, must be migrated to Actions
    @Nullable
    @Override
    public String getFlow() {
        return flow;
    }

    @Persisted
    public enum On {
        @JsonProperty("pr")
        @JsonAlias("PR")
        PR,
        @JsonProperty("commit")
        @JsonAlias("COMMIT")
        COMMIT
    }

    /**
     * Фильтр по тому, куда вливается pull-request или коммит
     */
    @Persisted
    public enum Into {
        /**
         * В транк
         */
        @JsonProperty("trunk")
        @JsonAlias("TRUNK")
        TRUNK(ArcBranch.Type.TRUNK),
        /**
         * В релизный бранч
         */
        @JsonProperty("release-branch")
        @JsonAlias("RELEASE_BRANCH")
        RELEASE_BRANCH(ArcBranch.Type.RELEASE_BRANCH);

        private final ArcBranch.Type acceptedBranchType;

        Into(ArcBranch.Type acceptedBranchType) {
            this.acceptedBranchType = acceptedBranchType;
        }

        public ArcBranch.Type getAcceptedBranchType() {
            return acceptedBranchType;
        }
    }

    @JsonIgnoreProperties(ignoreUnknown = true) // DEPRECATED: flow-vars, cleanup
    public static class Builder {

    }

}
