package ru.yandex.ci.core.config.a.model;

import java.util.List;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@JsonDeserialize(builder = FilterConfig.Builder.class)
public class FilterConfig {

    @Nonnull
    @JsonProperty
    Discovery discovery;

    @Nonnull
    @Singular
    @JsonProperty("st-queues")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> stQueues;

    @Nonnull
    @Singular
    @JsonProperty("not-st-queues")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> notStQueues;

    @Nonnull
    @Singular
    @JsonProperty("author-services")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> authorServices;

    @Nonnull
    @Singular
    @JsonProperty("not-author-services")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> notAuthorServices;

    @Nonnull
    @Singular
    @JsonProperty("not-authors")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> notAuthors;

    @Nonnull
    @Singular
    @JsonProperty("sub-paths")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> subPaths;

    @Nonnull
    @Singular
    @JsonProperty("not-sub-paths")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> notSubPaths;

    @Nonnull
    @Singular
    @JsonProperty("abs-paths")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> absPaths;

    @Nonnull
    @Singular
    @JsonProperty("not-abs-paths")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> notAbsPaths;

    @Nonnull
    @Singular
    @JsonProperty("feature-branches")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> featureBranches;

    @Nonnull
    @Singular
    @JsonProperty("not-feature-branches")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> notFeatureBranches;

    @Persisted
    public enum Discovery {
        @JsonProperty("default")
        @JsonAlias("DEFAULT")
        DEFAULT,
        @JsonProperty("any")
        @JsonAlias("ANY")
        ANY,
        @JsonProperty("dir")
        @JsonAlias("DIR")
        DIR,
        @JsonProperty("graph")
        @JsonAlias("GRAPH")
        GRAPH,
        @JsonProperty("pci-dss")
        @JsonAlias("PCI_DSS")
        PCI_DSS
    }

    public static class Builder {
        {
            discovery = Discovery.DEFAULT;
        }
    }

    public static FilterConfig defaultFilter() {
        return FilterConfig.builder()
                .discovery(FilterConfig.Discovery.DEFAULT)
                .build();
    }
}
