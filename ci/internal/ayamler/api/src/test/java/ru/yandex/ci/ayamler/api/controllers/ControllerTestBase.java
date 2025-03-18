package ru.yandex.ci.ayamler.api.controllers;

import java.util.List;
import java.util.concurrent.TimeUnit;

import io.grpc.BindableService;
import io.grpc.ManagedChannel;
import io.grpc.Server;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.AbstractBlockingStub;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.ayamler.AYamlerTestBase;

@SuppressWarnings("NotNullFieldNotInitialized")
public abstract class ControllerTestBase<T extends AbstractBlockingStub<T>> extends AYamlerTestBase {

    @Autowired
    private List<BindableService> controllers;

    private Server grpcServer;
    private ManagedChannel channel;
    protected T grpcClient;

    @BeforeEach
    public final void apiTestBaseSetUp() throws Exception {
        String name = InProcessServerBuilder.generateName();

        var inProcessServerBuilder = InProcessServerBuilder.forName(name);
        controllers.forEach(inProcessServerBuilder::addService);
        grpcServer = inProcessServerBuilder.build();
        grpcServer.start();

        channel = InProcessChannelBuilder.forName(name).directExecutor().build();
        grpcClient = createStub(channel);
    }

    @AfterEach
    public final void apiTestBaseTearDown() throws InterruptedException {
        grpcServer.shutdownNow();
        grpcServer.awaitTermination(3, TimeUnit.SECONDS);

        channel.shutdownNow();
        channel.awaitTermination(3, TimeUnit.SECONDS);
    }

    protected abstract T createStub(ManagedChannel channel);

}
