package ru.yandex.ci.core.arc;

import java.util.regex.Pattern;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.common.annotations.VisibleForTesting;
import lombok.Value;

import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;

/**
 * Просто ревизия в арке
 */
@Persisted
@Value(staticConstructor = "of")
@JsonSerialize(using = GsonJacksonSerializer.class)
@JsonDeserialize(using = GsonJacksonDeserializer.class)
public class ArcRevision implements CommitId {
    private static final Pattern ARC_REVISION_PATTERN = Pattern.compile("^[0-9a-f]{40}$|^r\\d{1,19}$");
    private static final Pattern SVN_REVISION_PATTERN = Pattern.compile("^\\d{1,19}$");  // Long.MAX_VALUE

    String commitId;

    @Override
    public String getCommitId() {
        return commitId;
    }

    public ArcCommit.Id toCommitId() {
        return ArcCommit.Id.of(commitId);
    }

    public OrderedArcRevision toOrdered(ArcBranch branch, long commitNumber, long pullRequestId) {
        return OrderedArcRevision.fromRevision(this, branch, commitNumber, pullRequestId);
    }

    public static ArcRevision of(CommitId commitId) {
        if (commitId instanceof ArcRevision arcRevision) {
            return arcRevision;
        }
        return new ArcRevision(commitId.getCommitId());
    }

    public static ArcRevision parse(String revision) {
        if (isArcRevision(revision)) {
            return ArcRevision.of(revision);
        }

        if (isSvnRevision(revision)) {
            return ArcRevision.of("r" + revision);
        }

        throw new BadRevisionFormatException("Revision has wrong format: " + revision);
    }

    @VisibleForTesting
    static boolean isSvnRevision(String revision) {
        return SVN_REVISION_PATTERN.matcher(revision).matches();
    }

    @VisibleForTesting
    static boolean isArcRevision(String revision) {
        return ARC_REVISION_PATTERN.matcher(revision).matches();
    }

    @Override
    public String toString() {
        return getCommitId();
    }

}
