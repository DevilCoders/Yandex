package ru.yandex.ci.client.storage;

import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;

import io.grpc.Server;
import io.grpc.stub.StreamObserver;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.StorageApi.CompareLargeTasksRequest;
import ru.yandex.ci.storage.api.StorageApi.CompareLargeTasksResponse;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc.StorageApiServiceImplBase;
import ru.yandex.ci.util.Clearable;
import ru.yandex.ci.util.MapsContainer;

public class StorageApiTestServer implements AutoCloseable, Clearable {

    private final MapsContainer maps = new MapsContainer();
    private final AtomicReference<CompareLargeTasksResponse> defaultLargeTests = new AtomicReference<>();
    private final Map<CompareLargeTasksRequest, CompareLargeTasksResponse> largeTests =
            maps.concurrentMap(defaultLargeTests);

    private final StorageApiServiceImplBase taskletService = new StorageApiServiceImplBase() {
        @Override
        public void compareLargeTasks(
                CompareLargeTasksRequest request,
                StreamObserver<CompareLargeTasksResponse> responseObserver
        ) {
            call(request, responseObserver, largeTests::get);
        }
    };

    private final Server server;

    public StorageApiTestServer(String serverName) {
        this.server = GrpcTestUtils.createAndStartServer(serverName, List.of(taskletService));
    }

    public void setDefaultCompareLargeTasks(CompareLargeTasksResponse response) {
        defaultLargeTests.set(response);
    }

    public void setCompareLargeTasks(CompareLargeTasksRequest request, CompareLargeTasksResponse response) {
        largeTests.put(request, response);
    }

    @Override
    public void clear() {
        defaultLargeTests.set(null);
        maps.clear();
    }

    @Override
    public void close() {
        server.shutdown();
    }

    private <Request, Response> void call(
            Request request,
            StreamObserver<Response> responseObserver,
            Function<Request, Response> impl
    ) {
        responseObserver.onNext(impl.apply(request));
        responseObserver.onCompleted();
    }
}
