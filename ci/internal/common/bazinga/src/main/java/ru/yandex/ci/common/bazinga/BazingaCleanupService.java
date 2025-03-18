package ru.yandex.ci.common.bazinga;

import java.time.Duration;
import java.time.Instant;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import org.springframework.scheduling.annotation.Scheduled;

@RequiredArgsConstructor
public class BazingaCleanupService {

    @Nonnull
    private final S3LogStorage s3LogStorage;

    @Nonnull
    private final Duration cleanupOlderThan;

    @Scheduled(
            fixedDelayString = "${bazinga.BazingaCleanupService.cleanupSeconds}",
            initialDelay = 0,
            timeUnit = TimeUnit.SECONDS
    )
    public void cleanup() {
        s3LogStorage.cleanup(cleanupOlderThan, Instant.now());
    }
}
