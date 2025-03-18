package ru.yandex.ci.common.bazinga.spring;

import java.io.IOException;
import java.net.BindException;
import java.net.ServerSocket;
import java.util.List;

import lombok.extern.slf4j.Slf4j;
import org.apache.curator.test.TestingServer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.commune.actor.typed.dynamic.DynamicConfigurationTypedServer;
import ru.yandex.commune.zk2.ZkConfiguration;
import ru.yandex.misc.ip.IpPort;

@Slf4j
@Configuration
public class TestZkConfig {

    @Bean(destroyMethod = "stop")
    public TestingServer zkTestingServer() throws Exception {
        int tryNumber = 0;
        while (true) {
            try {
                TestingServer server = new TestingServer();
                log.info("Server started");
                return server;
            } catch (BindException e) {
                if (tryNumber >= 10) {
                    throw e;
                }
                ++tryNumber;
            }
        }
    }

    @Bean
    public ZkConfiguration zkConfiguration(TestingServer testingServer) {
        return new ZkConfiguration(List.of("localhost"), testingServer.getPort());
    }

    @Bean
    public DynamicConfigurationTypedServer typedServer() throws IOException {
        int port;
        try (var socket = new ServerSocket(0)) {
            port = socket.getLocalPort();
            log.info("Random local port: {}", port);
        }
        return new DynamicConfigurationTypedServer(new IpPort(port));
    }
}
