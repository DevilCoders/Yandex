package ru.yandex.ci.engine.discovery;

import java.nio.file.Path;
import java.util.List;
import java.util.function.Predicate;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.Builder;
import lombok.Setter;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.util.CommitPathFetcherMemoized;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.discovery.ConfigChange;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.lang.NonNullApi;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder
@NonNullApi
public class DiscoveryContext {
    OrderedArcRevision revision;
    ArcRevision previousRevision;
    ArcCommit commit;
    ConfigChange configChange;
    ConfigBundle configBundle;
    @Nullable
    ArcBranch upstreamBranch;
    @With
    @Nullable
    ArcBranch featureBranch;
    DiscoveryType discoveryType;

    @Setter(AccessLevel.NONE)
    Supplier<List<String>> affectedPaths;

    @With
    @Nullable
    Predicate<FilterConfig> filterPredicate;

    public Path getConfigPath() {
        return configBundle.getConfigPath();
    }

    public ConfigEntity getConfigEntity() {
        return configBundle.getConfigEntity();
    }

    public List<String> getAffectedPaths() {
        return affectedPaths.get();
    }

    /**
     * Бранч, в который вливается PR
     *
     * @return null для любого изменения кроме pull-request
     */
    @Nullable
    public ArcBranch getUpstreamBranch() {
        return upstreamBranch;
    }

    /**
     * Бранч, из которого вливается PR
     *
     * @return null для любого изменения кроме pull-request из ARC
     */
    @Nullable
    public ArcBranch getFeatureBranch() {
        return featureBranch;
    }

    /**
     * Является ли ветка, на которой был затронут конфиг - транком
     *
     * @return true, если коммит уже в транке. false во всех других случаях, например это PR
     * или коммит из релизной или любой другой ветки
     */
    public boolean isTrunk() {
        return revision.getBranch().isTrunk();
    }

    public Predicate<FilterConfig> getFilterPredicate() {
        return filterPredicate != null ? filterPredicate : filter -> true;
    }

    public boolean isReleaseBranch() {
        return revision.getBranch().isRelease();
    }

    public static class Builder {

        public Builder affectedPathsProvider(ArcService arcService) {
            this.affectedPaths = new CommitPathFetcherMemoized(arcService, revision);
            return this;
        }

        public Builder affectedPaths(List<String> paths) {
            this.affectedPaths = () -> paths;
            return this;
        }

        public Builder affectedPaths(Supplier<List<String>> affectedPaths) {
            this.affectedPaths = affectedPaths;
            return this;
        }

        public Builder configChange(ConfigChangeType configChangeType) {
            this.configChange = new ConfigChange(configChangeType);
            return this;
        }
    }
}
