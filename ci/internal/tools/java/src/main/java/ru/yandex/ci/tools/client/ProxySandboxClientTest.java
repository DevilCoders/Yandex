package ru.yandex.ci.tools.client;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import com.google.common.io.ByteStreams;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.core.spring.clients.SandboxProxyClientConfig;
import ru.yandex.ci.engine.discovery.tier0.ChangesDetectorSandboxTaskResultParser;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(SandboxProxyClientConfig.class)
@Configuration
public class ProxySandboxClientTest extends AbstractSpringBasedApp {

    @Autowired
    ProxySandboxClient sandboxClient;

    @Override
    protected void run() throws IOException {
        try (var resource = sandboxClient.downloadResource(2280722368L)) {
            var stream = ChangesDetectorSandboxTaskResultParser.decompressAndUnTar("affected_targets.json.tar.gz",
                    resource.getStream());
            log.info("Resource: {}", new String(ByteStreams.toByteArray(stream), StandardCharsets.UTF_8));
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
