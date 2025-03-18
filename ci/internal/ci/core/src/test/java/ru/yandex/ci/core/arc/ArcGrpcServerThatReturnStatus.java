package ru.yandex.ci.core.arc;

import java.util.List;

import io.grpc.BindableService;
import io.grpc.Status;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;

import ru.yandex.arc.api.BranchServiceGrpc;
import ru.yandex.arc.api.CommitServiceGrpc;
import ru.yandex.arc.api.DiffServiceGrpc;
import ru.yandex.arc.api.FileServiceGrpc;
import ru.yandex.arc.api.HistoryServiceGrpc;
import ru.yandex.arc.api.Repo;

public class ArcGrpcServerThatReturnStatus {

    private final Status responseStatus;
    private final List<BindableService> services;

    public ArcGrpcServerThatReturnStatus(Status responseStatus) {
        this.responseStatus = responseStatus;
        services = List.of(
                new FileService(),
                new HistoryService(),
                new CommitService(),
                new DiffService(),
                new BranchService()
        );
    }

    public static InProcessServerBuilder inProcessServerBuilder(String serverName, Status unavailable) {
        var builder = InProcessServerBuilder.forName(serverName);
        new ArcGrpcServerThatReturnStatus(unavailable).getServices()
                .forEach(builder::addService);
        return builder;
    }

    public List<BindableService> getServices() {
        return services;
    }

    private <T> void responseWithError(StreamObserver<T> responseObserver) {
        responseObserver.onError(responseStatus.asRuntimeException());
    }

    private class FileService extends FileServiceGrpc.FileServiceImplBase {
        @Override
        public void readFile(Repo.ReadFileRequest request, StreamObserver<Repo.ReadFileResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void listDir(Repo.ListDirRequest request, StreamObserver<Repo.ListDirResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void stat(Repo.StatRequest request, StreamObserver<Repo.StatResponse> responseObserver) {
            responseWithError(responseObserver);
        }
    }

    private class HistoryService extends HistoryServiceGrpc.HistoryServiceImplBase {
        @Override
        public void log(Repo.LogRequest request, StreamObserver<Repo.LogResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void mergebase(Repo.MergebaseRequest request, StreamObserver<Repo.MergebaseResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void isAncestor(Repo.IsAncestorRequest request,
                               StreamObserver<Repo.IsAncestorResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void reflog(Repo.ReflogRequest request, StreamObserver<Repo.ReflogResponse> responseObserver) {
            responseWithError(responseObserver);
        }
    }

    private class CommitService extends CommitServiceGrpc.CommitServiceImplBase {
        @Override
        public void getCommit(Repo.GetCommitRequest request, StreamObserver<Repo.GetCommitResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void commitFile(Repo.CommitFileRequest request,
                               StreamObserver<Repo.CommitFileResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void cherryPick(Repo.CherryPickRequest request,
                               StreamObserver<Repo.CherryPickResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void revert(Repo.RevertRequest request, StreamObserver<Repo.RevertResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void rebase(Repo.RebaseRequest request, StreamObserver<Repo.RebaseResponse> responseObserver) {
            responseWithError(responseObserver);
        }
    }

    private class DiffService extends DiffServiceGrpc.DiffServiceImplBase {
        @Override
        public void blame(Repo.BlameRequest request, StreamObserver<Repo.BlameResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void changelist(Repo.ChangelistRequest request,
                               StreamObserver<Repo.ChangelistResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void diff(Repo.DiffRequest request, StreamObserver<Repo.DiffResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void diffstat(Repo.DiffstatRequest request, StreamObserver<Repo.DiffstatResponse> responseObserver) {
            responseWithError(responseObserver);
        }
    }

    private class BranchService extends BranchServiceGrpc.BranchServiceImplBase {
        @Override
        public void listRefs(Repo.ListRefsRequest request, StreamObserver<Repo.ListRefsResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void setRef(Repo.SetRefRequest request, StreamObserver<Repo.SetRefResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void deleteRef(Repo.DeleteRefRequest request, StreamObserver<Repo.DeleteRefResponse> responseObserver) {
            responseWithError(responseObserver);
        }

        @Override
        public void checkRefAccess(Repo.CheckRefAccessRequest request,
                                   StreamObserver<Repo.CheckRefAccessResponse> responseObserver) {
            responseWithError(responseObserver);
        }
    }

}
