package ru.yandex.ci.core.pr;

import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.gson.annotations.JsonAdapter;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcBranchAdapter;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@JsonSerialize(using = GsonJacksonSerializer.class)
@JsonDeserialize(using = GsonJacksonDeserializer.class)
public class PullRequestVcsInfo {

    /**
     * Ревизия на которой выполняется проверка. (mergeId в Arcanum'e.)
     */
    private final ArcRevision mergeRevision;
    /**
     * Ревизиия относительно которой выполняется проверка. Базовая ревизия в терминах автосборки. (to_id в Arcanum'e.)
     * HEAD ветки, в которую вливается pr, на момент создания итерации PR'а
     */
    private final ArcRevision upstreamRevision;

    /**
     * Ветка в которую делается PR
     */
    @JsonAdapter(ArcBranchAdapter.class)
    private final ArcBranch upstreamBranch;

    /**
     * Head ветки, из которой делается pr, на момент создания итерации PR'а (from_id в Arcanum'e.)
     */
    private final ArcRevision featureRevision;

    /**
     * Ветка из которой делается PR. Равна null, когда PR делается из свна.
     */
    @Nullable
    @JsonAdapter(ArcBranchAdapter.class)
    private final ArcBranch featureBranch;

    public PullRequestVcsInfo(ArcRevision mergeRevision, ArcRevision upstreamRevision, ArcBranch upstreamBranch,
                              ArcRevision featureRevision, @Nullable ArcBranch featureBranch) {
        this.mergeRevision = mergeRevision;
        this.upstreamRevision = upstreamRevision;
        this.upstreamBranch = upstreamBranch;
        this.featureRevision = featureRevision;
        this.featureBranch = featureBranch;
    }

    public ArcRevision getMergeRevision() {
        return mergeRevision;
    }

    public ArcRevision getUpstreamRevision() {
        return upstreamRevision;
    }

    public ArcBranch getUpstreamBranch() {
        return upstreamBranch;
    }

    public ArcRevision getFeatureRevision() {
        return featureRevision;
    }

    @Nullable
    public ArcBranch getFeatureBranch() {
        return featureBranch;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof PullRequestVcsInfo)) {
            return false;
        }
        PullRequestVcsInfo that = (PullRequestVcsInfo) o;
        return Objects.equals(mergeRevision, that.mergeRevision) &&
                Objects.equals(upstreamRevision, that.upstreamRevision) &&
                Objects.equals(upstreamBranch, that.upstreamBranch) &&
                Objects.equals(featureRevision, that.featureRevision) &&
                Objects.equals(featureBranch, that.featureBranch);
    }

    @Override
    public int hashCode() {
        return Objects.hash(mergeRevision, upstreamRevision, upstreamBranch, featureRevision, featureBranch);
    }

    @Override
    public String toString() {
        return "PullRequestVcsInfo{" +
                "mergeRevision=" + mergeRevision +
                ", upstreamRevision=" + upstreamRevision +
                ", upstreamBranch='" + upstreamBranch + '\'' +
                ", featureRevision=" + featureRevision +
                ", featureBranch='" + featureBranch + '\'' +
                '}';
    }
}
