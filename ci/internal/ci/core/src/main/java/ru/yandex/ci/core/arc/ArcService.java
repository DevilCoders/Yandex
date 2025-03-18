package ru.yandex.ci.core.arc;

import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;

public interface ArcService extends AutoCloseable {

    default Optional<ArcCommit> getLastCommit(Path path, CommitId startRevision) {
        var commits = getCommits(startRevision, null, path, 1);
        if (commits.isEmpty()) {
            return Optional.empty();
        }
        return Optional.of(commits.get(0))
                .map(ArcCommitWithPath::getArcCommit);
    }

    default List<ArcCommit> getCommits(CommitId startRevision,
                                       @Nullable CommitId earlierStopRevision,
                                       int limit) {
        return getCommits(startRevision, earlierStopRevision, null, limit).stream()
                .map(ArcCommitWithPath::getArcCommit)
                .collect(Collectors.toList());
    }

    /**
     * Загружает коммиты от указанной ревизии
     *
     * @param laterStartRevision  inclusive
     * @param earlierStopRevision exclusive
     * @param path                path in arc repository
     * @param limit               max number of commits
     * @return list of commits
     */
    List<ArcCommitWithPath> getCommits(CommitId laterStartRevision,
                                       @Nullable CommitId earlierStopRevision,
                                       @Nullable Path path,
                                       int limit);

    /**
     * Возвращает все коммиты от указанной ревизии до начала ветки, включая коммит, от которого отведена ветка
     *
     * @param startRevision начальная ревизия
     * @param path          фильтр пути, отображает только коммиты, которые затрагивали переданный путь
     * @return список коммитов в порядке от свежих к более ранним до первого коммита из транка, включая его
     */
    List<ArcCommit> getBranchCommits(CommitId startRevision, @Nullable Path path);

    ArcCommit getCommit(CommitId revision);

    boolean isCommitExists(CommitId revision);

    ArcRevision getLastRevisionInBranch(ArcBranch branch);

    default void processChanges(CommitId laterFromCommit,
                                @Nullable CommitId earlierToCommit,
                                Consumer<Repo.ChangelistResponse.Change> changeConsumer) {
        processChanges(laterFromCommit, earlierToCommit, null, changeConsumer);
    }

    void processChanges(CommitId laterFromCommit,
                        @Nullable CommitId earlierToCommit,
                        @Nullable Path pathFilterDir,
                        Consumer<Repo.ChangelistResponse.Change> changeConsumer);

    default boolean isFileExists(Path path, CommitId revision) {
        return getStat(path.toString(), revision, false)
                .filter(it -> it.getType() == RepoStat.EntryType.FILE)
                .isPresent();
    }

    default Optional<String> getFileContent(Path path, CommitId revision) {
        return getFileContent(path.toString(), revision);
    }

    Optional<String> getFileContent(String path, CommitId revision);

    Optional<RepoStat> getStat(String path, CommitId revision, boolean lastChanged);

    CompletableFuture<Optional<RepoStat>> getStatAsync(String path, CommitId revision, boolean lastChanged);

    void createReleaseBranch(CommitId commitId, ArcBranch branch, String oauthToken);

    List<String> getBranches(String branchPrefix);

    Optional<ArcRevision> getMergeBase(CommitId a, CommitId b);

    List<Path> listDir(String path, CommitId commitId, boolean recursive, boolean onlyFiles);

    void resetBranch(ArcBranch branch, CommitId commitId, String oauthToken);

    Shared.Commit commitFile(String path,
                             ArcBranch branch,
                             CommitId expectedHead,
                             String message,
                             String content,
                             String token
    );
}
