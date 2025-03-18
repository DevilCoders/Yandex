package ru.yandex.ci.core.arc;

import java.io.IOException;
import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import org.assertj.core.api.InstanceOfAssertFactories;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.RegisterExtension;

import ru.yandex.arc.api.FileServiceGrpc.FileServiceImplBase;
import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Repo.ReadFileRequest;
import ru.yandex.arc.api.Repo.ReadFileResponse;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.common.grpc.GrpcCleanupExtension;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.api.Assertions.assertThrows;

class ArcServiceTest {

    @RegisterExtension
    public GrpcCleanupExtension grpcCleanup = new GrpcCleanupExtension();

    @Test
    void branchCommits() {
        ArcService arcService = new ArcServiceCanonImpl(false);

        List<ArcCommit> commits = arcService.getBranchCommits(ArcRevision.of(
                "09759a29821fbf8c771ecaae4db80f22f208cf79"), null);

        assertThat(commits)
                .extracting(ArcCommit::getMessage)
                .extracting(m -> m.lines().findFirst().orElseThrow())
                .containsExactly(
                        "CI-914 one more change",
                        "CI-914 more changes to release branch",
                        "CI-914 trigger",
                        "update" // trunk commit
                );
    }

    @Test
    void isCommitExists_whenDoesNotExist() {
        ArcService arcService = new ArcServiceCanonImpl(false);
        /* Arc responses with
        "status": {
            "code": "NOT_FOUND",
            "description": "arc/server/public.cpp:280: id not found aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        }*/
        assertThat(arcService.isCommitExists(ArcRevision.of("a".repeat(40)))).isFalse();

        /* Arc responses with
        "status": {
            "code": "INVALID_ARGUMENT",
            "description": "arc/server/public.cpp:1636: Can't parse revision:
                { Code: NoSuchBranch Message: \"NoSuchBranch\" } NoSuchBranch"
        }*/
        assertThrows(
                RuntimeException.class,
                () -> arcService.isCommitExists(ArcRevision.of("branch-that-doesn't exists"))
        );
    }

    @Test
    void list_getContent() {
        ArcService arcService = new ArcServiceCanonImpl(false);
        var revision = ArcRevision.of("4c65c83836e35ccfa274779f59526e07fe7168ae");


        var registryRoot = Path.of("ci/registry");
        var paths = arcService.listDir(registryRoot.toString(), revision, true, true);

        assertThat(paths).hasSize(369);

        var inCommonCi = paths.stream()
                .filter(p -> p.startsWith("common/ci"))
                .map(Path::toString)
                .collect(Collectors.toList());

        assertThat(inCommonCi).containsExactly(
                "common/ci/ci_storage_register.yaml",
                "common/ci/find_ci_check.yaml"
        );

        var findCiCheckContent = arcService.getFileContent(registryRoot.resolve(inCommonCi.get(1)), revision);
        assertThat(findCiCheckContent).isNotEmpty()
                .get(InstanceOfAssertFactories.STRING)
                .isNotBlank()
                .isEqualTo(TestUtils.textResource("sample-find_ci_check.yaml"));
    }

    @Test
    void getStatAsync() throws Exception {
        var arcService = buildInProcessArcService(new FileServiceImplBase() {
            @Override
            public void stat(Repo.StatRequest request, StreamObserver<Repo.StatResponse> responseObserver) {
                responseObserver.onNext(Repo.StatResponse.newBuilder()
                        .setName(request.getPath())
                        .setType(Shared.TreeEntryType.TreeEntryFile)
                        .setFileOid(request.getRevision())
                        .build()
                );
                responseObserver.onCompleted();
            }
        });
        Optional<RepoStat> stat = arcService.getStatAsync("path/to/file", ArcRevision.of("a".repeat(40)), false)
                .get();
        assertThat(stat).get()
                .isEqualTo(new RepoStat(
                        "path/to/file", RepoStat.EntryType.FILE,
                        0, false, false, "a".repeat(40), null, false
                ));
    }

    @Test
    void getStatAsync_whenPathNotFound() throws Exception {
        var arcService = buildInProcessArcService(new FileServiceImplBase() {
            @Override
            public void stat(Repo.StatRequest request, StreamObserver<Repo.StatResponse> responseObserver) {
                responseObserver.onError(GrpcUtils.notFoundException(
                        "path not found"
                ));
            }
        });
        Optional<RepoStat> stat = arcService.getStatAsync("path/to/file", ArcRevision.of("a".repeat(40)), false)
                .get();
        assertThat(stat).isEmpty();
    }

    @Test
    void getFileContent_whenRequestedPathIsDir() throws IOException {
        var arcService = buildInProcessArcService(new FileServiceImplBase() {
            @Override
            public void readFile(ReadFileRequest request, StreamObserver<ReadFileResponse> responseObserver) {
                responseObserver.onError(GrpcUtils.invalidArgumentException("requested path is not a file"));
            }
        });
        assertThat(arcService.getFileContent("path/to/dir", ArcRevision.of("a".repeat(40))))
                .isEmpty();
    }

    private ArcService buildInProcessArcService(FileServiceImplBase fileServiceMock) throws IOException {
        String serverName = InProcessServerBuilder.generateName();
        grpcCleanup.register(
                InProcessServerBuilder
                        .forName(serverName)
                        .directExecutor()
                        .addService(fileServiceMock)
                        .build()
        ).start();
        var properties = GrpcClientPropertiesStub.of(serverName, grpcCleanup);
        return new ArcServiceImpl(properties, null);
    }

}
