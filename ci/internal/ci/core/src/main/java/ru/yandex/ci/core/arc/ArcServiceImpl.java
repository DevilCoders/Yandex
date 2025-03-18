package ru.yandex.ci.core.arc;

import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;
import java.util.function.Supplier;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.annotation.ParametersAreNonnullByDefault;

import com.google.common.base.Preconditions;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.protobuf.ByteString;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.util.unit.DataSize;

import ru.yandex.arc.api.BranchServiceGrpc;
import ru.yandex.arc.api.CommitServiceGrpc;
import ru.yandex.arc.api.DiffServiceGrpc;
import ru.yandex.arc.api.FileServiceGrpc;
import ru.yandex.arc.api.HistoryServiceGrpc;
import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.proto.StreamObserverUtils;
import ru.yandex.ci.util.ExceptionUtils;

import static ru.yandex.ci.core.arc.ArcServiceProtoMappers.toArcCommitWithPath;
import static ru.yandex.ci.core.arc.ArcServiceProtoMappers.toRepoStat;

@ParametersAreNonnullByDefault
public class ArcServiceImpl implements ArcService {

    private static final Logger log = LoggerFactory.getLogger(ArcServiceImpl.class);

    /**
     * В этом классе собран весь треш и угар по работе с арком, что бы в остальных частях кода был идеальный порядок
     * Когда-нибудь у нас будет нормальное API и станет лучше https://st.yandex-team.ru/ARC-1897
     */
    private static final long MAX_FILE_SIZE_BYTES = DataSize.ofMegabytes(5).toBytes();
    private static final int LOG_BATCH_SIZE = 100;

    private static final int COMMIT_CACHE_SIZE = 100_000;
    private static final Duration COMMIT_CACHE_EXPIRATION = Duration.ofDays(1);

    private final GrpcClient grpcClient;
    private final Supplier<FileServiceGrpc.FileServiceBlockingStub> fileService;
    private final Supplier<FileServiceGrpc.FileServiceStub> fileServiceAsync;
    private final Supplier<HistoryServiceGrpc.HistoryServiceBlockingStub> historyService;
    private final Supplier<CommitServiceGrpc.CommitServiceBlockingStub> commitService;
    private final Supplier<DiffServiceGrpc.DiffServiceBlockingStub> diffService;
    private final Supplier<BranchServiceGrpc.BranchServiceBlockingStub> branchService;

    private final boolean processChangesAndSkipNotFoundCommits;

    private final LoadingCache<RawCommitId, ArcCommit> commitCache;

    public ArcServiceImpl(
            @Nonnull GrpcClientProperties grpcClientProperties,
            @Nullable MeterRegistry meterRegistry
    ) {
        this(grpcClientProperties, meterRegistry, false);
    }

    public ArcServiceImpl(
            @Nonnull GrpcClientProperties grpcClientProperties,
            @Nullable MeterRegistry meterRegistry,
            boolean processChangesAndSkipNotFoundCommits
    ) {
        this(
                GrpcClientImpl.builder(grpcClientProperties, ArcServiceImpl.class)
                        .excludeLoggingFullResponse(BranchServiceGrpc.getListRefsMethod())
                        .excludeLogging(FileServiceGrpc.getStatMethod())
                        .build(),
                meterRegistry,
                processChangesAndSkipNotFoundCommits
        );
    }

    public ArcServiceImpl(
            @Nonnull GrpcClient grpcClient,
            @Nullable MeterRegistry meterRegistry,
            boolean processChangesAndSkipNotFoundCommits
    ) {
        this.grpcClient = grpcClient;
        fileService = grpcClient.buildStub(FileServiceGrpc::newBlockingStub);
        fileServiceAsync = grpcClient.buildStub(FileServiceGrpc::newStub);
        historyService = grpcClient.buildStub(HistoryServiceGrpc::newBlockingStub);
        commitService = grpcClient.buildStub(CommitServiceGrpc::newBlockingStub);
        diffService = grpcClient.buildStub(DiffServiceGrpc::newBlockingStub);
        branchService = grpcClient.buildStub(BranchServiceGrpc::newBlockingStub);

        commitCache = CacheBuilder.newBuilder()
                .expireAfterAccess(COMMIT_CACHE_EXPIRATION)
                .maximumSize(COMMIT_CACHE_SIZE)
                .recordStats()
                .build(CacheLoader.from(this::doGetCommit));

        if (meterRegistry != null) {
            GuavaCacheMetrics.monitor(meterRegistry, commitCache, "commit-cache");
        }

        this.processChangesAndSkipNotFoundCommits = processChangesAndSkipNotFoundCommits;
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }

    @Override
    public List<ArcCommitWithPath> getCommits(CommitId laterStartRevision,
                                              @Nullable CommitId earlierStopRevision,
                                              @Nullable Path path,
                                              int limit) {

        var commits = new ArrayList<ArcCommitWithPath>();

        Repo.LogRequest.Builder requestBuilder = Repo.LogRequest.newBuilder()
                .addStartRevisions(laterStartRevision.getCommitId())
                .setLimit(limit)
                .setFullTopo(false);

        if (earlierStopRevision != null) {
            requestBuilder.addAllHideRevisions(List.of(
                    earlierStopRevision.getCommitId()
            ));
        }
        if (path != null) {
            requestBuilder.setPath(path.toString());
        }

        Iterator<Repo.LogResponse> responseIterator = historyService.get().log(requestBuilder.build());

        while (responseIterator.hasNext()) {
            for (Repo.LogResponse.LogItem logItem : responseIterator.next().getCommitsList()) {
                commits.add(toArcCommitWithPath(logItem.getCommit(), logItem.getPath()));
            }

        }
        return commits;
    }

    @Value
    private static class LogBatch {
        List<ArcCommit> commits;
        @Nullable
        CommitId next;
    }

    @Override
    public List<ArcCommit> getBranchCommits(final CommitId startRevision, @Nullable Path path) {
        Preconditions.checkNotNull(startRevision);
        List<ArcCommit> commits = new ArrayList<>();

        CommitId offset = startRevision;
        while (offset != null) {
            LogBatch batch = arcLogBatch(offset, path, LOG_BATCH_SIZE);

            for (ArcCommit arcCommit : batch.getCommits()) {
                commits.add(arcCommit);
                if (arcCommit.isTrunk()) {
                    return commits;
                }
            }

            if (batch.getCommits().isEmpty()) {
                return commits;
            }
            offset = batch.getNext();
        }

        throw new RuntimeException("trunk commit not found from revision: " + startRevision);
    }

    private LogBatch arcLogBatch(CommitId startRevision, @Nullable Path path, int limit) {
        Preconditions.checkArgument(limit > 0);
        Repo.LogRequest.Builder requestBuilder = Repo.LogRequest.newBuilder()
                .addStartRevisions(startRevision.getCommitId())
                .setLimit(limit + 1)
                .setFullTopo(false);

        if (path != null) {
            requestBuilder.setPath(path.toString());
        }

        var responseIterator = historyService.get().log(requestBuilder.build());
        var commits = new ArrayList<ArcCommit>();

        while (responseIterator.hasNext()) {
            for (Repo.LogResponse.LogItem logItem : responseIterator.next().getCommitsList()) {
                Shared.Commit commit = logItem.getCommit();
                var arcCommit = toArcCommitWithPath(commit, logItem.getPath()).getArcCommit();
                commits.add(arcCommit);
            }
        }

        if (commits.isEmpty()) {
            return new LogBatch(Collections.emptyList(), null);
        }
        if (commits.size() <= limit) {
            return new LogBatch(commits, null);
        }
        int lastIndex = commits.size() - 1;
        return new LogBatch(commits.subList(0, lastIndex), commits.get(lastIndex));
    }


    @Override
    public ArcCommit getCommit(CommitId revision) {
        try {
            return commitCache.get(RawCommitId.of(revision));
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    @Override
    public boolean isCommitExists(CommitId revision) {
        try {
            getCommit(revision);
        } catch (CommitNotFoundException e) {
            return false;
        }
        return true;
    }

    private ArcCommit doGetCommit(CommitId revision) {
        try {
            return ArcServiceProtoMappers.toArcCommit(
                    commitService.get().getCommit(
                            Repo.GetCommitRequest.newBuilder()
                                    .setRevision(revision.getCommitId())
                                    .build()
                    ).getCommit()
            );
        } catch (StatusRuntimeException e) {
            switch (e.getStatus().getCode()) {
                case NOT_FOUND -> throw CommitNotFoundException.fromCommitId(revision);
                case INVALID_ARGUMENT -> throw new RuntimeException("Invalid argument for arc", e);
                default -> throw e;
            }
        }
    }

    @Override
    public ArcRevision getLastRevisionInBranch(ArcBranch branch) {
        Repo.GetCommitRequest request = Repo.GetCommitRequest.newBuilder()
                .setRevision(branch.asString())
                .build();

        Repo.GetCommitResponse response = commitService.get().getCommit(request);
        return ArcRevision.of(response.getCommit().getOid());
    }

    @Override
    public void processChanges(CommitId laterFromCommit,
                               @Nullable CommitId earlierToCommit,
                               @Nullable Path pathFilterDir,
                               Consumer<Repo.ChangelistResponse.Change> changeConsumer) {
        var request = Repo.ChangelistRequest.newBuilder()
                // в diff api from и to понятия инвертированы относительно наших
                // from в diff api - старше to
                .setToRevision(laterFromCommit.getCommitId())
                .addAllPathFilter(pathFilterDir == null ? List.of() : List.of(toPathFilter(pathFilterDir)));
        if (earlierToCommit != null) {
            request.setFromRevision(earlierToCommit.getCommitId());
        }

        Iterator<Repo.ChangelistResponse> responseIterator = diffService.get().changelist(request.build());

        try {
            while (responseIterator.hasNext()) {
                Repo.ChangelistResponse changelistResponse = responseIterator.next();
                for (Repo.ChangelistResponse.Change change : changelistResponse.getChangesList()) {
                    changeConsumer.accept(change);
                }
            }
        } catch (StatusRuntimeException e) {
            if (!processChangesAndSkipNotFoundCommits || e.getStatus().getCode() != Status.Code.NOT_FOUND) {
                throw e;
            }
            log.error("process changes failed: laterFromCommit {}, earlierToCommit {}, pathFilterDir {}",
                    laterFromCommit, earlierToCommit, pathFilterDir, e);
        }
    }

    private String toPathFilter(Path pathDir) {
        return pathDir + "/";
    }

    @Override
    public Optional<String> getFileContent(String path, CommitId revision) {
        Repo.ReadFileRequest request = Repo.ReadFileRequest.newBuilder()
                .setRevision(revision.getCommitId())
                .setPath(path)
                .setLimitBytes(MAX_FILE_SIZE_BYTES)
                .build();

        Iterator<Repo.ReadFileResponse> responseIterator;
        try {
            responseIterator = fileService.get().readFile(request);
            responseIterator.next(); // header
        } catch (StatusRuntimeException e) {
            if (e.getStatus().getCode() == Status.Code.NOT_FOUND
                    // arc returns INVALID_ARGUMENT when requested path is dir
                    || e.getStatus().getCode() == Status.Code.INVALID_ARGUMENT) {
                return Optional.empty();
            }
            throw e;
        }
        ByteString result = null;
        while (responseIterator.hasNext()) {
            ByteString next = responseIterator.next().getData();
            result = (result == null) ? next : result.concat(next);
            Preconditions.checkState(
                    result.size() < MAX_FILE_SIZE_BYTES,
                    "File %s:%s size more than %s bytes",
                    path, revision.getCommitId(), MAX_FILE_SIZE_BYTES
            );
        }
        Preconditions.checkState(result != null,
                "File %s:s cannot be null",
                path, revision.getCommitId());
        return Optional.of(result.toStringUtf8());
    }

    @Override
    public List<Path> listDir(String path, CommitId commitId, boolean recursive, boolean onlyFiles) {
        Repo.ListDirRequest request = Repo.ListDirRequest.newBuilder()
                .setRevision(commitId.getCommitId())
                .setPath(path)
                .setRecursive(recursive)
                .build();
        Optional<Iterator<Repo.ListDirResponse>> response = Optional.empty();
        try {
            response = Optional.of(fileService.get().listDir(request));
        } catch (StatusRuntimeException exception) {
            log.error("Can't list directory {} {}", path, commitId.getCommitId());
        }
        var paths = new ArrayList<Path>();
        if (response.isPresent()) {
            for (var i = response.get(); i.hasNext(); ) {
                for (var listDir : i.next().getEntriesList()) {
                    if (onlyFiles && listDir.getType() == Shared.TreeEntryType.TreeEntryFile) {
                        paths.add(Path.of(listDir.getName()));
                    }
                }
            }
        }
        return paths;
    }

    @Override
    public Optional<RepoStat> getStat(String path, CommitId revision, boolean lastChanged) {
        var request = Repo.StatRequest.newBuilder()
                .setRevision(revision.getCommitId())
                .setPath(path)
                .setLastChanged(lastChanged)
                .build();

        Repo.StatResponse stat;
        try {
            stat = fileService.get().stat(request);
        } catch (StatusRuntimeException e) {
            if (e.getStatus().getCode() == Status.Code.NOT_FOUND) {
                return Optional.empty();
            }
            throw e;
        }
        return Optional.of(toRepoStat(revision, path, stat));
    }

    @Override
    @SuppressWarnings("FutureReturnValueIgnored")
    public CompletableFuture<Optional<RepoStat>> getStatAsync(String path, CommitId revision, boolean lastChanged) {
        var resultFuture = new CompletableFuture<Optional<RepoStat>>();

        var request = Repo.StatRequest.newBuilder()
                .setRevision(revision.getCommitId())
                .setPath(path)
                .setLastChanged(lastChanged)
                .build();
        var stub = fileServiceAsync.get();
        StreamObserverUtils.invokeAndGetSingleValueFuture(stub::stat, request)
                .thenApply(stat -> toRepoStat(revision, path, stat))
                .thenApply(Optional::of)
                .whenComplete((optionalStat, t) -> {
                    if (t == null) {
                        resultFuture.complete(optionalStat);
                        return;
                    }
                    try {
                        var unwrapped = ExceptionUtils.unwrap(t);
                        if (unwrapped instanceof StatusRuntimeException) {
                            if (((StatusRuntimeException) unwrapped).getStatus().getCode() == Status.Code.NOT_FOUND) {
                                resultFuture.complete(Optional.empty());
                                return;
                            }
                        }
                        resultFuture.completeExceptionally(new RuntimeException(
                                "Failed getStatAsync(%s): %s".formatted(request, unwrapped.getMessage()), unwrapped
                        ));
                    } catch (Exception e) {
                        t.addSuppressed(e);
                        resultFuture.completeExceptionally(new RuntimeException(
                                "Failed getStatAsync(%s): %s".formatted(request, t.getMessage()), t
                        ));
                    }
                });
        return resultFuture;
    }

    @Override
    public void createReleaseBranch(CommitId commitId, ArcBranch branch, String oauthToken) {
        Preconditions.checkArgument(!branch.isPr(), "branch %s is a pr", branch);
        Preconditions.checkArgument(!branch.isTrunk(), "branch %s is trunk", branch);

        Repo.SetRefRequest.Builder request = Repo.SetRefRequest.newBuilder();
        request.setBranch(branch.asString());
        request.setCommitOid(commitId.getCommitId());

        //noinspection ResultOfMethodCallIgnored
        branchService.get().withCallCredentials(new OAuthCallCredentials(oauthToken))
                .setRef(request.build());
    }

    @Override
    public List<String> getBranches(String branchPrefix) {
        var result = new ArrayList<String>(10240);

        Iterator<Repo.ListRefsResponse> responseIterator =
                branchService.get().listRefs(
                        Repo.ListRefsRequest.newBuilder()
                                .setPrefixFilter(branchPrefix)
                                .setInconsistent(true)
                                .setLightweight(true)
                                .build()
                );

        while (responseIterator.hasNext()) {
            Repo.ListRefsResponse response = responseIterator.next();
            for (Repo.ListRefsResponse.Ref ref : response.getRefsList()) {
                result.add(ref.getName());
            }
        }

        return result;
    }

    @Override
    public Optional<ArcRevision> getMergeBase(CommitId a, CommitId b) {
        try {
            Repo.MergebaseResponse response = historyService.get().mergebase(Repo.MergebaseRequest.newBuilder()
                    .addHeadRevisions(a.getCommitId())
                    .addHeadRevisions(b.getCommitId())
                    .build());

            Preconditions.checkState(response.getBasesCount() == 1);

            return Optional.of(ArcRevision.of(response.getBases(0).getCommitOid()));

        } catch (StatusRuntimeException e) {
            if (e.getMessage().contains("TStatus_ECode_NoBaseCommit")) {
                log.info("No merge base between {} and {} found", a.getCommitId(), b.getCommitId());
                return Optional.empty();
            }
            throw e;
        }
    }

    @Override
    public void resetBranch(ArcBranch branch, CommitId commitId, String oauthToken) {
        Preconditions.checkState(branch.isUser());

        var response = branchService.get().withCallCredentials(new OAuthCallCredentials(oauthToken))
                .setRef(Repo.SetRefRequest.newBuilder()
                        .setBranch(branch.asString())
                        .setCommitOid(commitId.getCommitId())
                        .setForce(true)
                        .build());
        var reflogRecord = response.getReflogRecord();
        Preconditions.checkState(reflogRecord.getAfterCommit().getOid().equals(commitId.getCommitId()));
        log.info("Moved head branch {} from {} to {}", branch,
                reflogRecord.getBeforeCommit().getOid(),
                reflogRecord.getAfterCommit().getOid()
        );
    }

    @Override
    public Shared.Commit commitFile(
            String path,
            ArcBranch branch,
            CommitId expectedHead,
            String message,
            String content,
            String token
    ) {
        Preconditions.checkState(branch.isUser() || branch.isRelease() || branch.isGroup());
        var response = commitService.get()
                .withCallCredentials(new OAuthCallCredentials(token))
                .commitFile(
                        Repo.CommitFileRequest.newBuilder()
                                .setPath(path)
                                .setBranch(branch.asString())
                                .setMessage(message)
                                .setData(ByteString.copyFromUtf8(content))
                                .setExpectedCommitOid(expectedHead.getCommitId())
                                .build()
                );

        return response.getCommit();
    }

    @Value
    private static class RawCommitId implements CommitId {
        String commitId;

        @Override
        public String getCommitId() {
            return commitId;
        }

        static RawCommitId of(CommitId commitId) {
            return new RawCommitId(commitId.getCommitId());
        }
    }
}
