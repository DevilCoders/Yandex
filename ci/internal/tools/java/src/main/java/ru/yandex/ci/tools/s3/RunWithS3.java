package ru.yandex.ci.tools.s3;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Optional;

import lombok.extern.slf4j.Slf4j;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.util.StreamUtils;

import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.bazinga.spring.S3LogStorageConfig;
import ru.yandex.ci.flow.engine.runtime.bazinga.FlowTask;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.util.HostnameUtils;
import ru.yandex.commune.bazinga.impl.FullJobId;

@Slf4j
@Import(S3LogStorageConfig.class)
@Configuration
public class RunWithS3 extends AbstractSpringBasedApp {

    @Autowired
    S3LogStorage s3;

    @Override
    protected void run() throws IOException {
        s3.setOnetimeTaskChecker(taskId -> Optional.of(Mockito.mock(FlowTask.class)));

        var currentHost = HostnameUtils.getHostname();
        var jobId = FullJobId.parse("flow/20211028T143333.913Z/-782498556");

        var location = s3.getLocation(currentHost, jobId);
        Files.createDirectories(location.getLocalPath().getParent());
        Files.writeString(location.getLocalPath(), "Hello, username");

        s3.compressAndUpload(location);

        try (var stream = s3.getInputStream(location).orElseThrow().getInput()) {
            var text = StreamUtils.copyToString(stream, StandardCharsets.UTF_8);
            log.info("{}", text);
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
