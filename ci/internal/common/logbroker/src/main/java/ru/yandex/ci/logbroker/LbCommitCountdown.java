package ru.yandex.ci.logbroker;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.function.BiConsumer;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.kikimr.persqueue.consumer.StreamListener;

@Slf4j
public class LbCommitCountdown extends MessagesCountdown {
    private final long cookie;

    private final Clock clock;
    private final Instant created;
    private final StreamListener.ReadResponder responder;
    private final BiConsumer<StreamListener.ReadResponder, Long> commitCallback;

    public LbCommitCountdown(
            long cookie,
            Clock clock,
            StreamListener.ReadResponder responder,
            int messagesCount,
            BiConsumer<StreamListener.ReadResponder, Long> commitCallback
    ) {
        super(messagesCount);
        this.cookie = cookie;
        this.clock = clock;
        this.created = clock.instant();
        this.responder = responder;
        this.commitCallback = commitCallback;
    }

    @Override
    protected void onAllMessagesProcessed() {
        log.debug(
                "0 messages left to commit cookie {}, will commit. Processing time: {}",
                cookie, Duration.between(created, clock.instant())
        );
        this.commitCallback.accept(this.responder, this.cookie);
    }

    @Override
    public String toString() {
        return "LbCommitCountdown{cookie: " + cookie + ", " + super.toString() + "}";
    }

    public long getCookie() {
        return cookie;
    }

    public Instant getCreated() {
        return created;
    }
}
