package ru.yandex.ci.core.project;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.config.a.model.ReleaseBranchesConfig.DefaultConfigSource;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.lang.NonNullApi;

@SuppressWarnings("ReferenceEquality")
@JsonIgnoreProperties(ignoreUnknown = true) // permissions
@Persisted
@Value
@Builder(toBuilder = true)
@NonNullApi
@Nonnull
@JsonDeserialize(using = GsonJacksonDeserializer.class)
@JsonSerialize(using = GsonJacksonSerializer.class)
public class ReleaseConfigState {
    String releaseId;
    String title;

    @Nullable
    String description;

    @Nullable
    AutoReleaseConfigState auto;

    @Nullable
    AutoReleaseConfigState branchesAuto;

    boolean releaseFromTrunkForbidden;
    boolean releaseBranchesEnabled;
    boolean defaultConfigFromBranch;

    @Singular
    List<String> tags;

    public static ReleaseConfigState of(ReleaseConfig releaseConfig) {
        var builder = ReleaseConfigState.builder()
                .releaseId(releaseConfig.getId())
                .title(releaseConfig.getTitle())
                .description(releaseConfig.getDescription())
                .auto(AutoReleaseConfigState.of(releaseConfig.getAuto()))
                .tags(releaseConfig.getTags());

        if (releaseConfig.getBranches().isEnabled()) {
            builder.releaseBranchesEnabled(true);
            builder.branchesAuto(AutoReleaseConfigState.of(releaseConfig.getBranches().getAuto()));
            builder.releaseFromTrunkForbidden(releaseConfig.getBranches().isForbidTrunkReleases());
            builder.defaultConfigFromBranch(
                    releaseConfig.getBranches().getDefaultConfigSource() == DefaultConfigSource.BRANCH);
        } else {
            builder.releaseFromTrunkForbidden(false);
        }

        return builder.build();
    }

    public List<String> getTags() {
        return Objects.requireNonNullElse(tags, List.of());
    }

    public AutoReleaseConfigState getAuto() {
        return Objects.requireNonNullElse(auto, AutoReleaseConfigState.EMPTY);
    }

    public AutoReleaseConfigState getBranchesAuto() {
        return Objects.requireNonNullElse(branchesAuto, AutoReleaseConfigState.EMPTY);
    }
}
