package ru.yandex.ci.api;

import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import com.google.common.base.Preconditions;
import com.google.common.hash.HashCode;
import com.google.common.hash.Hashing;
import io.grpc.ManagedChannel;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.stub.AbstractBlockingStub;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.arc.api.Repo.ChangelistResponse.ChangeType;
import ru.yandex.ci.api.internal.security.DelegateTokenRequest;
import ru.yandex.ci.api.internal.security.DelegateTokenResponse;
import ru.yandex.ci.api.internal.security.SecurityApiGrpc;
import ru.yandex.ci.api.spring.CiApiGrpcConfig;
import ru.yandex.ci.api.spring.CiApiGrpcTestConfig;
import ru.yandex.ci.api.spring.CiApiPropertiesTestConfig;
import ru.yandex.ci.client.arcanum.util.RevisionNumberPullRequestIdPair;
import ru.yandex.ci.common.bazinga.spring.S3LogStorageTestConfig;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.spring.jobs.TrackerTestConfig;
import ru.yandex.ci.flow.spring.TestJobsConfig;

import static ru.yandex.ci.core.test.TestData.DIFF_SET_1;
import static ru.yandex.ci.core.test.TestData.DIFF_SET_2;
import static ru.yandex.ci.core.test.TestData.DIFF_SET_3;
import static ru.yandex.ci.core.test.TestData.TRUNK_R3;
import static ru.yandex.ci.core.test.TestData.TRUNK_R4;
import static ru.yandex.ci.core.test.TestData.TRUNK_R5;

@ContextConfiguration(classes = {
        CiApiGrpcConfig.class,

        CiApiGrpcTestConfig.class,
        CiApiPropertiesTestConfig.class,
        S3LogStorageTestConfig.class,
        TestJobsConfig.class,
        TrackerTestConfig.class
})
public abstract class ControllerTestBase<T extends AbstractBlockingStub<T>> extends EngineTestBase {

    private ManagedChannel channel;

    @Autowired
    protected String serverName;

    @Autowired
    private BranchService branchService;

    protected SecurityApiGrpc.SecurityApiBlockingStub grpcSecurityApi;

    protected T grpcService;

    @BeforeEach
    public final void apiTestBaseSetUp() {
        channel = InProcessChannelBuilder.forName(serverName).directExecutor().build();
        grpcService = createStub(channel);
        grpcSecurityApi = SecurityApiGrpc.newBlockingStub(channel);

        arcServiceStub.resetAndInitTestData();

        arcanumTestServer.mockGetReviewRequestBySvnRevision(
                RevisionNumberPullRequestIdPair.of(TRUNK_R3.getNumber(), DIFF_SET_1.getPullRequestId()),
                RevisionNumberPullRequestIdPair.of(TRUNK_R4.getNumber(), DIFF_SET_2.getPullRequestId()),
                RevisionNumberPullRequestIdPair.of(TRUNK_R5.getNumber(), DIFF_SET_3.getPullRequestId())
        );
        mockValidationSuccessful(); // TODO: CI-2865
    }

    @AfterEach
    public final void apiTestBaseTearDown() throws InterruptedException {
        channel.shutdownNow();
        channel.awaitTermination(3, TimeUnit.SECONDS);
    }

    protected ManagedChannel getChannel() {
        return channel;
    }

    protected abstract T createStub(ManagedChannel channel);

    protected Optional<ArcCommit> doCommit(OrderedArcRevision revision, Map<Path, ChangeType> changes) {
        Preconditions.checkArgument(revision.getBranch().isTrunk());
        ArcCommit commit = TestData.toCommit(revision, TestData.CI_USER);
        db.currentOrTx(() -> db.arcCommit().save(commit));
        return arcServiceStub.addTrunkCommit(
                commit,
                "releases/repo/rev99",
                changes
        );
    }

    protected OrderedArcRevision createAndPrepareConfiguration(int revisionNumber, Path... configDirs) {
        return createAndPrepareConfiguration(revisionNumber, true, configDirs);
    }

    protected OrderedArcRevision createAndPrepareConfiguration(
            int revisionNumber,
            boolean delegate,
            Path... configDirs) {
        OrderedArcRevision revision = revision(revisionNumber);

        // TODO: переделать на нормальные каталоги
        var map = new LinkedHashMap<Path, ChangeType>();
        for (var dir : configDirs) {
            map.put(dir.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME), ChangeType.Add);
        }
        map.put(VirtualCiProcessId.LARGE_FLOW.getPath(), ChangeType.Add);
        map.put(Path.of("ci/registry/dummy/task.yaml"), ChangeType.Add);

        doCommit(revision, map);

        discoveryServicePostCommits.processPostCommit(revision.getBranch(), revision.toRevision(), false);

        mockSandboxDelegationAny();

        if (delegate) {
            mockYavAny();
            var list = new ArrayList<String>();
            for (var dir : configDirs) {
                list.add(dir.toString());
            }
            list.add(VirtualCiProcessId.LARGE_FLOW.getDir());
            for (var dir : list) {
                var response = grpcSecurityApi.delegateToken(DelegateTokenRequest.newBuilder()
                        .setConfigDir(dir)
                        .setRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                        .build()
                );
                Preconditions.checkState(response.equals(DelegateTokenResponse.getDefaultInstance()));
            }
        }
        return revision;
    }

    protected void discoverCommits(ArcBranch branch, OrderedArcRevision... commits) {
        for (var rev : commits) {
            discoveryServicePostCommits.processPostCommit(branch, rev.toRevision(), false);
        }
    }

    protected Branch createBranchAt(ArcCommit commit, CiProcessId processId) {
        return db.currentOrTx(() -> branchService.createBranch(
                processId,
                commit.toOrderedTrunkArcRevision(),
                TestData.CI_USER
        ));
    }

    protected static OrderedArcRevision revision(int number) {
        return revision(number, ArcBranch.trunk());
    }

    protected static OrderedArcRevision revision(int number, ArcBranch branch) {
        HashCode hashCode = Hashing.sha1().hashString(
                "commit-" + number + "-" + branch.getBranch(),
                StandardCharsets.UTF_8
        );
        return OrderedArcRevision.fromHash(hashCode.toString(), branch.asString(), number, 0);
    }

}
