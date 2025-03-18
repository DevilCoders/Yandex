package ru.yandex.ci.storage.core.logbroker;

import java.time.Clock;
import java.util.Comparator;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerStreamListener;

@Slf4j
public class StreamListenerCommitWatchdog {
    private final Clock clock;
    private final LogbrokerStreamListener listener;
    private final int timeoutSeconds;

    public StreamListenerCommitWatchdog(Clock clock, LogbrokerStreamListener listener, int timeoutSeconds) {
        this.clock = clock;
        this.listener = listener;
        this.timeoutSeconds = timeoutSeconds;
        var thread = new Thread(this::loop, "lb-commit-watchdog");
        thread.setDaemon(true);
        thread.start();
    }

    private void loop() {
        while (!Thread.interrupted()) {
            try {
                execute();
            } catch (RuntimeException e) {
                log.error("Error", e);
            }

            try {
                //noinspection BusyWait
                Thread.sleep(TimeUnit.MINUTES.toMillis(1));
            } catch (InterruptedException e) {
                return;
            }
        }
    }

    private void execute() {
        log.info("Checking for stuck reads");
        var cut = clock.instant().minusSeconds(timeoutSeconds);
        var notCommited = listener.getNotCommitedReads().stream().filter(x -> x.getCreated().isBefore(cut)).toList();

        if (notCommited.isEmpty()) {
            return;
        }

        var limit = 32;
        log.warn(
                "Found {} stuck reads, top({}):\n{}",
                notCommited.size(),
                limit,
                notCommited.stream()
                        .sorted(Comparator.comparing(LbCommitCountdown::getCreated))
                        .limit(limit)
                        .map(LbCommitCountdown::toString)
                        .collect(Collectors.joining("\n"))
        );
    }
}
