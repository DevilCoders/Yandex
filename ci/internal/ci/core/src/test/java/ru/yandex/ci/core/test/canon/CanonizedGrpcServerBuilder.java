package ru.yandex.ci.core.test.canon;

import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

import io.grpc.BindableService;
import io.grpc.ManagedChannel;
import io.grpc.Server;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.AbstractBlockingStub;

/**
 * Создает локальный grpc сервер, который формирует ответы из локальных файлов.
 * Позволяет сделать дамп ответов реального сервера и использовать их в тестах.
 */
public class CanonizedGrpcServerBuilder {

    private final List<GrpcServiceRef> serviceRefs = new ArrayList<>();
    private Path dataPath;
    private boolean canonize = false;

    private CanonizedGrpcServerBuilder() {
    }

    public static CanonizedGrpcServerBuilder builder() {
        return new CanonizedGrpcServerBuilder();
    }

    public CanonizedGrpcServerBuilder withDataPath(Path path) {
        this.dataPath = path;
        return this;
    }

    /**
     * Произвести канонизацию ответов реального сервера.
     */
    public CanonizedGrpcServerBuilder doCanonize(boolean canonize) {
        this.canonize = canonize;
        return this;
    }

    /**
     * Регистрирует сервис
     *
     * @param stub     клиент, для похода в реальный сервер. Используется только при канонизации.
     * @param implBase базовый класс сервиса на сервере
     */
    public CanonizedGrpcServerBuilder withService(
            AbstractBlockingStub<?> stub,
            Class<? extends BindableService> implBase
    ) {
        serviceRefs.add(new GrpcServiceRef(stub, implBase));
        return this;
    }

    public ManagedChannel buildAndStart() throws IOException, ReflectiveOperationException {
        if (dataPath == null) {
            dataPath = Path.of(CanonizedGrpcServerBuilder.class.getResource("/grpc-canon-data").getPath());
        }
        if (canonize) {
            System.err.println(banner(dataPath));
        }
        String name = InProcessServerBuilder.generateName();
        Server mockServer;
        InProcessServerBuilder inProcessServerBuilder = InProcessServerBuilder.forName(name);
        for (GrpcServiceRef serviceRef : serviceRefs) {
            BindableService serviceProxy = CachedGrpcService.makeProxy(
                    serviceRef.getStub(), serviceRef.getImplBase(), dataPath, canonize
            );

            inProcessServerBuilder.addService(serviceProxy);
        }

        mockServer = inProcessServerBuilder.build();
        mockServer.start();

        return InProcessChannelBuilder.forName(name).directExecutor().build();
    }

    private static String banner(Path canonDataPath) {
        return String.format(
                "************************************************\n" +
                        "*\n" +
                        "*      Canonization is enabled\n" +
                        "*      Copy canonical data from\n" +
                        "*      %s\n" +
                        "*      To related source path\n" +
                        "*\n" +
                        "************************************************",
                canonDataPath.toAbsolutePath()
        );
    }

    private static class GrpcServiceRef {
        private final AbstractBlockingStub<?> stub;
        private final Class<? extends BindableService> implBase;

        private GrpcServiceRef(AbstractBlockingStub<?> stub, Class<? extends BindableService> implBase) {
            this.stub = stub;
            this.implBase = implBase;
        }

        public AbstractBlockingStub<?> getStub() {
            return stub;
        }

        public Class<? extends BindableService> getImplBase() {
            return implBase;
        }
    }
}
