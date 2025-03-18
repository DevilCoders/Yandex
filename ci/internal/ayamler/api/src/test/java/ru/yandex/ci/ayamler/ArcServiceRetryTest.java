package ru.yandex.ci.ayamler;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.URI;
import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.grpc.Attributes;
import io.grpc.EquivalentAddressGroup;
import io.grpc.NameResolver;
import io.grpc.Server;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import io.grpc.netty.NettyServerBuilder;
import io.grpc.stub.StreamObserver;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.core.io.Resource;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.arc.api.FileServiceGrpc;
import ru.yandex.arc.api.Repo;
import ru.yandex.ci.ayamler.api.spring.ArcConfig;
import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.spring.CommonConfig;

import static org.assertj.core.api.Assertions.assertThat;

@Slf4j
@ContextConfiguration(classes = {
        ArcServiceRetryTest.Config.class,
})
public class ArcServiceRetryTest extends AYamlerTestBase {

    private static final Map<String, AtomicInteger> REQUEST_COUNT = new ConcurrentHashMap<>();

    @Autowired
    ArcService arcServiceClient;

    @BeforeEach
    void cleanUpRequestCount() {
        REQUEST_COUNT.clear();
    }

    @Test
    void getStat_shouldRetryStatusUnavailable() {
        try {
            arcServiceClient.getStat("path", ArcRevision.of("rev"), false);
        } catch (StatusRuntimeException e) {
            // noop
        }
        checkRetryCount("FileService/Stat");
    }

    @Test
    void getStatAsync_shouldRetryStatusUnavailable() {
        try {
            arcServiceClient.getStatAsync("path", ArcRevision.of("rev"), false).get();
        } catch (Exception e) {
            // noop
        }
        checkRetryCount("FileService/Stat");
    }

    @Test
    void getFileContent_shouldRetryStatusUnavailable() {
        try {
            arcServiceClient.getFileContent("path", ArcRevision.of("rev"));
        } catch (StatusRuntimeException e) {
            // noop
        }
        checkRetryCount("FileService/ReadFile");
    }

    private void checkRetryCount(String method) {
        /* We use `hasValueGreaterThan` and not `hasValue`, cause number one requests are changed from one test run
           to another. This test is potentially flaky */
        var server1Count = REQUEST_COUNT.get("server1/" + method);
        var server2Count = REQUEST_COUNT.get("server2/" + method);
        assertThat(server1Count).hasValueGreaterThan(0);
        assertThat(server2Count).hasValueGreaterThan(0);
        assertThat(server1Count.get() + server2Count.get()).isEqualTo(10);
    }

    private static Server buildServer(String serverName, Status responseStatus,
                                      Map<String, AtomicInteger> requestCount) {
        return NettyServerBuilder.forPort(0)
                .executor(
                        Executors.newFixedThreadPool(
                                1,
                                new ThreadFactoryBuilder()
                                        .setDaemon(true)
                                        .setNameFormat("ArcServiceRetryTest-" + serverName + "-%d")
                                        .build()
                        )
                )
                .addService(new FileServiceThatResponsesWithStatusUnavailable(responseStatus, requestCount, serverName))
                .build();
    }

    private static class FileServiceThatResponsesWithStatusUnavailable extends FileServiceGrpc.FileServiceImplBase {

        private final Status responseStatus;
        private final Map<String, AtomicInteger> requestCount;
        private final String serverName;

        private FileServiceThatResponsesWithStatusUnavailable(Status responseStatus,
                                                              Map<String, AtomicInteger> requestCount,
                                                              String serverName) {
            this.responseStatus = responseStatus;
            this.requestCount = requestCount;
            this.serverName = serverName;
        }

        @Override
        public void stat(Repo.StatRequest request, StreamObserver<Repo.StatResponse> responseObserver) {
            var key = serverName + "/FileService/Stat";
            log.info("Processing {}", key);
            requestCount.computeIfAbsent(key, k -> new AtomicInteger()).incrementAndGet();
            responseObserver.onError(responseStatus.asRuntimeException());
        }

        @Override
        public void readFile(Repo.ReadFileRequest request, StreamObserver<Repo.ReadFileResponse> responseObserver) {
            var key = serverName + "/FileService/ReadFile";
            log.info("Processing {}", key);
            requestCount.computeIfAbsent(key, k -> new AtomicInteger()).incrementAndGet();
            responseObserver.onError(responseStatus.asRuntimeException());
        }

    }

    private static class NameResolverFactory extends NameResolver.Factory {

        private final Server[] servers;

        private NameResolverFactory(Server... servers) {
            this.servers = servers;
        }

        @Override
        public String getDefaultScheme() {
            return "dns";
        }

        @Override
        public NameResolver newNameResolver(URI targetUri, NameResolver.Args args) {
            return new NameResolver() {
                @Override
                public String getServiceAuthority() {
                    return "dns";
                }

                @Override
                public void shutdown() {
                }

                @Override
                public void start(Listener listener) {
                    var addresses = Stream.of(servers)
                            .map(server -> new EquivalentAddressGroup(new InetSocketAddress(
                                    "localhost", server.getPort()
                            )))
                            .collect(Collectors.toList());
                    log.info("Resolving {}", addresses);
                    listener.onAddresses(addresses, Attributes.EMPTY);
                }
            };
        }
    }

    @Configuration
    @Import({
            CommonConfig.class,
            ArcConfig.class
    })
    static class Config {

        @Bean
        public GrpcClient arcServiceChannel(
                @Value("${ayamler.arcServiceChannel.endpoint}") String endpoint,
                @Value("${ayamler.arcServiceChannel.connectTimeout}") Duration connectTimeout,
                @Value("classpath:arcServiceChannel.grpcServiceConfig.json") Resource grpcServiceConfig,
                List<Server> servers
        ) {
            var nameResolverFactory = new NameResolverFactory(servers.toArray(Server[]::new));

            var properties = GrpcClientProperties.builder()
                    .endpoint(endpoint)
                    .connectTimeout(connectTimeout)
                    .maxRetryAttempts(10)
                    .grpcServiceConfig(grpcServiceConfig)
                    .configurer(builder -> builder.nameResolverFactory(nameResolverFactory))
                    .build();

            return ArcConfig.arcServiceChannelBuilder(properties);
        }

        @Bean(destroyMethod = "shutdown")
        Server server1() throws IOException {
            return buildServer("server1", Status.UNAVAILABLE, REQUEST_COUNT).start();
        }

        @Bean(destroyMethod = "shutdown")
        Server server2() throws IOException {
            return buildServer("server2", Status.UNAVAILABLE, REQUEST_COUNT).start();
        }

    }
}
