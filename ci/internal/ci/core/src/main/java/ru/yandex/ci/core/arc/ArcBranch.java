package ru.yandex.ci.core.arc;

import java.util.Objects;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;

/**
 * Имя ветки. Используется исключительно как указатель на ветку.
 * Метаинформация в {@link BranchInfo}
 */
@Persisted
@Value
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@JsonSerialize(using = GsonJacksonSerializer.class)
@JsonDeserialize(using = GsonJacksonDeserializer.class)
public class ArcBranch {

    private static final ArcBranch TRUNK = new ArcBranch("trunk", Type.TRUNK, -1);
    private static final ArcBranch UNKNOWN = new ArcBranch("", ArcBranch.Type.UNKNOWN, 0);

    public static final String PR_BRANCH_PREFIX = "pr:";
    private static final String RELEASE_PREFIX = "releases/";
    private static final String USERS_PREFIX = "users/";
    private static final String GROUPS_PREFIX = "groups/";

    String branch;
    Type type;
    long pullRequestId;

    public Type getType() {
        return type;
    }

    public long getPullRequestId() {
        Preconditions.checkState(
                type == Type.PR,
                "Not PR branch %s", asString()
        );
        return pullRequestId;
    }

    public String asString() {
        return branch;
    }

    public boolean isTrunk() {
        return type == Type.TRUNK;
    }

    public boolean isPr() {
        return type == Type.PR;
    }

    public boolean isRelease() {
        return type == Type.RELEASE_BRANCH;
    }

    public boolean isUser() {
        return type == Type.USER_BRANCH;
    }

    public boolean isGroup() {
        return type == Type.GROUP_BRANCH;
    }

    public boolean isUnknown() {
        return type == Type.UNKNOWN;
    }

    public boolean isReleaseOrUser() {
        return isRelease() || isUser();
    }

    public void checkBranchIsReleaseOrUser() {
        // Only branches of this class could be tracker in database (for Timeline or Branch entities)
        Preconditions.checkState(isReleaseOrUser(),
                "Internal error. Branch %s has invalid type %s, must be either %s or %s",
                branch, type, Type.RELEASE_BRANCH, Type.USER_BRANCH);
    }

    public static ArcBranch ofPullRequest(long pullRequestId) {
        return new ArcBranch(PR_BRANCH_PREFIX + pullRequestId, Type.PR, pullRequestId);
    }

    public static ArcBranch ofString(String branch) {
        Preconditions.checkNotNull(branch);
        if (branch.startsWith(PR_BRANCH_PREFIX)) {
            long pullRequestId = Long.parseLong(branch.substring(PR_BRANCH_PREFIX.length()));
            return ofPullRequest(pullRequestId);
        }
        return ofBranchName(branch);
    }

    public static ArcBranch ofBranchName(String name) {
        Preconditions.checkNotNull(name);
        Preconditions.checkArgument(!name.isEmpty());

        if (name.equals(TRUNK.asString())) {
            return trunk();
        }
        if (name.startsWith(RELEASE_PREFIX)) {
            return new ArcBranch(name, Type.RELEASE_BRANCH, -1);
        }
        if (name.startsWith(USERS_PREFIX)) {
            return new ArcBranch(name, Type.USER_BRANCH, -1);
        }
        if (name.startsWith(GROUPS_PREFIX)) {
            return new ArcBranch(name, Type.GROUP_BRANCH, -1);
        }
        return new ArcBranch(name, Type.UNKNOWN, -1);
    }

    public static ArcBranch trunk() {
        return TRUNK;
    }

    public static ArcBranch unknown() {
        return UNKNOWN;
    }

    @Persisted
    public enum Type {
        TRUNK,
        /**
         * мета-бранч - пулл-реквесты. Физически существует только в arcanum.
         */
        PR,
        RELEASE_BRANCH,
        USER_BRANCH,
        GROUP_BRANCH,
        UNKNOWN
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof ArcBranch arcBranch)) {
            return false;
        }
        return Objects.equals(branch, arcBranch.branch);
    }

    @Override
    public int hashCode() {
        return Objects.hash(branch);
    }

    @Override
    public String toString() {
        return asString();
    }
}
