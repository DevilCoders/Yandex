package ru.yandex.ci.storage.tests;

import java.nio.file.Path;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitWithPath;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.RepoStat;

public class TestsArcService implements ArcService {
    public static final String PR_ONE_REVISION = "0396111a6031d8c0e0641e41a8627d6434973e13";
    public static final String PR_TWO_REVISION = "249edde49bbe368864aef8b62968a09820c705f0";
    public static final String PR_THREE_REVISION = "aa7e931de9e9af9d91cf4a2aad67d3ee14307eb9";

    private final Map<CommitId, ArcCommit> commits =
            Stream.concat(
                            IntStream.range(1, 10).mapToObj(
                                    number -> ArcCommit.builder()
                                            .id(ArcCommit.Id.of("r" + number))
                                            .svnRevision(number)
                                            .createTime(Instant.EPOCH.plusSeconds(number))
                                            .author("user42")
                                            .message("Test commit")
                                            .build()
                            ),
                            Stream.of(
                                    ArcCommit.builder()
                                            .id(ArcCommit.Id.of(PR_ONE_REVISION))
                                            .svnRevision(0)
                                            .createTime(Instant.EPOCH)
                                            .author("user42")
                                            .message("Pr commit")
                                            .build(),
                                    ArcCommit.builder()
                                            .id(ArcCommit.Id.of(PR_TWO_REVISION))
                                            .svnRevision(0)
                                            .createTime(Instant.EPOCH)
                                            .author("user42")
                                            .message("Pr commit")
                                            .build(),
                                    ArcCommit.builder()
                                            .id(ArcCommit.Id.of(PR_THREE_REVISION))
                                            .svnRevision(0)
                                            .createTime(Instant.EPOCH)
                                            .author("user42")
                                            .message("Pr commit")
                                            .build()
                            ))
                    .collect(Collectors.toMap(x -> ArcRevision.of(x.getCommitId()), Function.identity()));

    @Override
    public List<ArcCommitWithPath> getCommits(
            CommitId laterStartRevision,
            @Nullable CommitId earlierStopRevision,
            @Nullable Path path, int limit
    ) {
        return List.of();
    }

    @Override
    public List<ArcCommit> getBranchCommits(CommitId startRevision, @Nullable Path path) {
        return List.of();
    }

    @Override
    public ArcCommit getCommit(CommitId revision) {
        return commits.get(revision);
    }

    @Override
    public boolean isCommitExists(CommitId revision) {
        return false;
    }

    @Override
    public ArcRevision getLastRevisionInBranch(ArcBranch branch) {
        return null;
    }

    @Override
    public void processChanges(CommitId laterFromCommit, CommitId earlierToCommit, @Nullable Path pathFilterDir,
                               Consumer<Repo.ChangelistResponse.Change> changeConsumer) {

    }

    @Override
    public Optional<String> getFileContent(String path, CommitId revision) {
        return Optional.empty();
    }

    @Override
    public Optional<RepoStat> getStat(String path, CommitId revision, boolean lastChanged) {
        return Optional.empty();
    }

    @Override
    public CompletableFuture<Optional<RepoStat>> getStatAsync(String path, CommitId revision, boolean lastChanged) {
        return null;
    }

    @Override
    public void createReleaseBranch(CommitId commitId, ArcBranch branch, String oauthToken) {

    }

    @Override
    public List<String> getBranches(String branchPrefix) {
        return List.of();
    }

    @Override
    public Optional<ArcRevision> getMergeBase(CommitId a, CommitId b) {
        return Optional.empty();
    }

    @Override
    public List<Path> listDir(String path, CommitId commitId, boolean recursive, boolean onlyFiles) {
        return List.of();
    }

    @Override
    public void resetBranch(ArcBranch branch, CommitId commitId, String oauthToken) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Shared.Commit commitFile(String path, ArcBranch branch, CommitId expectedHead, String message,
                                    String content, String token) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void close() throws Exception {

    }
}
