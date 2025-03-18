package ru.yandex.ci.tools.client;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.google.common.base.Stopwatch;
import com.google.common.collect.Lists;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.io.IOUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.ayamler.AYamlerClient;
import ru.yandex.ci.client.ayamler.StrongModeRequest;
import ru.yandex.ci.storage.core.spring.AYamlerClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.util.ResourceUtils;

@Slf4j
@Import(AYamlerClientConfig.class)
@Configuration
public class AYamlerClientTest extends AbstractSpringBasedApp {

    @Autowired
    AYamlerClient aYamlerClientImpl;

    @Override
    protected void run() throws Exception {
        var url = ResourceUtils.url("AYamlerClientTest.getStrongModeBatch.txt");
        List<String> paths;
        try (var inputStream = url.openStream()) {
            paths = List.of(
                    IOUtils.toString(inputStream, StandardCharsets.UTF_8)
                            .split("\n")
            );
        }

        List<String> revisions = List.of("72973230203f8b6bba8a847fcc05b7822ecfed9c");

        for (var revision : revisions) {
            for (var partPaths : Lists.partition(paths, 200)) {
                var stopWatch = Stopwatch.createStarted();
                log.info("Requesting {} path/revision pairs", partPaths.size());
                var futureResponse = aYamlerClientImpl.getStrongMode(
                        partPaths.stream()
                                .map(path -> new StrongModeRequest(path, revision, "robot-ci"))
                                .collect(Collectors.toSet())
                );
                futureResponse.get();
                log.info("Requested {} path/revision pairs within {} msec", partPaths.size(),
                        stopWatch.elapsed(TimeUnit.MILLISECONDS));
            }
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
