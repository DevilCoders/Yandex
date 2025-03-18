package ru.yandex.ci.core.arc;

import java.nio.file.Path;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.common.proto.ProtoConverter;

@Slf4j
public class ArcServiceProtoMappers {

    // Standard Arc attribute
    private static final String ATTRIBUTE_PULL_REQUEST = "pr.id";

    private ArcServiceProtoMappers() {
    }

    static RepoStat toRepoStat(CommitId revision, String path, Repo.StatResponse stat) {
        return new RepoStat(
                stat.getName(),
                switch (stat.getType()) {
                    case TreeEntryDir -> RepoStat.EntryType.DIR;
                    case TreeEntryFile -> RepoStat.EntryType.FILE;
                    case TreeEntryNone -> RepoStat.EntryType.NONE;
                    case UNRECOGNIZED -> throw new IllegalArgumentException(
                            "failed to get %s at %s: unexpected node type %s"
                                    .formatted(path, revision.getCommitId(), stat.getType())
                    );
                },
                stat.getSize(),
                stat.getExecutable(),
                stat.getSymlink(),
                stat.getFileOid(),
                stat.hasLastChanged() ? toArcCommit(stat.getLastChanged()) : null,
                stat.getEncrypted()
        );
    }

    @SuppressWarnings("ProtoTimestampGetSecondsGetNano")
    static ArcCommit toArcCommit(Shared.Commit commit) {
        return ArcCommit.builder()
                .id(ArcCommit.Id.of(commit.getOid()))
                .author(commit.getAuthor())
                .message(commit.getMessage())
                .createTime(ProtoConverter.convert(commit.getTimestamp()))
                .parents(commit.getParentOidsList())
                .svnRevision(commit.getSvnRevision())
                .pullRequestId(getPullRequestId(commit))
                .build();
    }

    static long getPullRequestId(Shared.Commit commit) {
        if (commit.getAttributesCount() > 0) {
            for (var attr : commit.getAttributesList()) {
                if (ATTRIBUTE_PULL_REQUEST.equals(attr.getName())) {
                    try {
                        return Long.parseLong(attr.getValue());
                    } catch (NumberFormatException nfe) {
                        log.error("Unable to parse commit [{}] number: [{}]",
                                commit.getOid(), attr);
                    }
                }
            }
        }
        return 0;
    }

    static ArcCommitWithPath toArcCommitWithPath(Shared.Commit commit, @Nullable String path) {
        return new ArcCommitWithPath(toArcCommit(commit), (path != null) ? Path.of(path) : null);
    }

}
