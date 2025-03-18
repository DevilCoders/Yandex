package ru.yandex.ci.core.arc;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;

/**
 * Упорядочная ревизия в арке. Сам арк не всегда гарантирует порядок, поэтому часто порядок присваивает сам CI
 */
@Persisted
@Value
@JsonDeserialize(using = GsonJacksonDeserializer.class)
@JsonSerialize(using = GsonJacksonSerializer.class)
public class OrderedArcRevision implements CommitId {

    String commitId;

    /**
     * Может быть чем угодно - бранчем, PR-ом и и т.д. Просто некий идентификатор ветки arc
     */
    ArcBranch branch;

    /**
     * Порядковый номер комита в бранче.
     * <ul>
     * <li>В транке равен SVN ревизии.</li>
     * <li>В пулл реквестре равен diffSetId</li>
     * <li>В ветке - присвоенный порядковый номер, начиная от транка <br>(см. {@link RevisionNumberService})</li>
     * </ul>
     * Гарантируется строго монотонное возрастание и уникальность в рамках бранча.
     * Непрерывность НЕ гарантируется.
     * <p>
     * Не используйте number для сравнения порядка ревизий! Ревизии могут находиться в разных ветках, и не иметь
     * отношения порядка. Используйте {@link #isBefore(OrderedArcRevision)} и аналогичные, там есть защита от таких
     * ошибок.
     */
    long number;

    /**
     * Код PR, если доступен в коммите
     */
    long pullRequestId;

    private OrderedArcRevision(String hash, String branch, long number, long pullRequestId) {
        this(hash, ArcBranch.ofString(branch), number, pullRequestId);
    }

    private OrderedArcRevision(String hash, ArcBranch branch, long number, long pullRequestId) {
        this.commitId = Preconditions.checkNotNull(hash);
        this.branch = Preconditions.checkNotNull(branch);
        this.number = number;
        this.pullRequestId = pullRequestId;
    }

    public static OrderedArcRevision fromRevision(
            ArcRevision revision,
            String branch,
            long number,
            long pullRequestId) {
        return new OrderedArcRevision(revision.getCommitId(), branch, number, pullRequestId);
    }

    public static OrderedArcRevision fromRevision(
            ArcRevision revision,
            ArcBranch branch,
            long number,
            long pullRequestId) {
        return new OrderedArcRevision(revision.getCommitId(), branch, number, pullRequestId);
    }

    @JsonCreator
    public static OrderedArcRevision fromHash(String hash, String branch, long number, long pullRequestId) {
        return new OrderedArcRevision(hash, branch, number, pullRequestId);
    }

    public static OrderedArcRevision fromHash(String hash, ArcBranch branch, long number, long pullRequestId) {
        return new OrderedArcRevision(hash, branch, number, pullRequestId);
    }

    @Override
    public String getCommitId() {
        return commitId;
    }

    public ArcRevision toRevision() {
        return ArcRevision.of(commitId);
    }

    public ArcCommit.Id toCommitId() {
        return ArcCommit.Id.of(commitId);
    }

    public boolean hasSvnRevision() {
        return branch.isTrunk() && number > 0;
    }

    // true, если текущая ревизия раньше переданной аргументом
    public boolean isBefore(OrderedArcRevision other) {
        throwIfRevisionFromDifferentBranch(other);
        if (getNumber() == other.getNumber()) {
            throwIfCommitIdIsNotEqual(other);
        }
        return getNumber() < other.getNumber();
    }

    public boolean fromSameBranch(OrderedArcRevision other) {
        return getBranch().equals(other.getBranch());
    }

    public boolean isBeforeOrSame(OrderedArcRevision other) {
        throwIfRevisionFromDifferentBranch(other);
        return isSame(other) || getNumber() < other.getNumber();
    }

    public boolean isSame(OrderedArcRevision other) {
        throwIfRevisionFromDifferentBranch(other);
        if (getNumber() == other.getNumber()) {
            throwIfCommitIdIsNotEqual(other);
            return true;
        }
        return false;
    }

    private void throwIfRevisionFromDifferentBranch(OrderedArcRevision other) {
        Preconditions.checkArgument(
                fromSameBranch(other),
                "Cannot compare revisions from different branches. Current: %s, arg: %s",
                this,
                other
        );
    }

    private void throwIfCommitIdIsNotEqual(OrderedArcRevision other) {
        Preconditions.checkArgument(getCommitId().equals(other.getCommitId()),
                "Revisions %s and %s have same numbers, but different ids",
                this, other);
    }
}
