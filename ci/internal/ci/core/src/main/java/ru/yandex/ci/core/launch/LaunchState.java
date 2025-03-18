package ru.yandex.ci.core.launch;

import java.time.Instant;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.lang.NonNullApi;

@Value
@Nonnull
@NonNullApi
public class LaunchState {

    @Nullable
    @Column(dbType = DbType.UTF8)
    String flowLaunchId;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant started;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant finished;

    @Column(dbType = DbType.STRING)
    LaunchState.Status status;

    @Column(dbType = DbType.UTF8)
    String text;

    public LaunchState(@Nullable String flowLaunchId, @Nullable Instant started, @Nullable Instant finished,
                       Status status, String text) {
        this.flowLaunchId = flowLaunchId;
        this.started = started;
        this.finished = finished;
        this.status = status;
        this.text = text;
    }

    public LaunchState(Status status) {
        this(status, "");
    }

    public LaunchState(Status status, String text) {
        this((String) null, null, null, status, text);
    }


    @Persisted
    public enum Status {
        // вносишь правки - поправь документацию
        DELAYED,
        POSTPONE,
        STARTING(Flag.PROCESSING),
        RUNNING(Flag.PROCESSING),
        RUNNING_WITH_ERRORS(Flag.PROCESSING),
        FAILURE,
        WAITING_FOR_MANUAL_TRIGGER,
        WAITING_FOR_STAGE,
        WAITING_FOR_SCHEDULE(Flag.PROCESSING),
        SUCCESS(Flag.TERMINAL),
        CANCELED(Flag.TERMINAL),
        CANCELLING(Flag.PROCESSING),
        WAITING_FOR_CLEANUP,
        CLEANING(Flag.PROCESSING),
        IDLE;

        private final boolean terminal;
        private final boolean processing;

        private static final Set<Status> TERMINAL_STATUES = Stream.of(values())
                .filter(Status::isTerminal)
                .collect(Collectors.toUnmodifiableSet());

        private static final Set<Status> NON_TERMINAL_STATUES = Stream.of(values())
                .filter(s -> !s.isTerminal())
                .collect(Collectors.toUnmodifiableSet());

        Status(Flag... flags) {
            this.terminal = Set.of(flags).contains(Flag.TERMINAL);
            this.processing = Set.of(flags).contains(Flag.PROCESSING);
        }

        public static Set<Status> terminalStatuses() {
            return TERMINAL_STATUES;
        }

        public static Set<Status> nonTerminalStatuses() {
            return NON_TERMINAL_STATUES;
        }

        public boolean isTerminal() {
            return terminal;
        }

        /**
         * ВАЖНО: !processing не эквивалентно terminal. Не выполняющийся флоу может ожидать действия пользователя.
         */
        public boolean isProcessing() {
            return processing;
        }

        private enum Flag {
            TERMINAL, PROCESSING
        }
    }

    public Optional<String> getFlowLaunchId() {
        return Optional.ofNullable(flowLaunchId);
    }

    public Optional<Instant> getStarted() {
        return Optional.ofNullable(started);
    }

    public Optional<Instant> getFinished() {
        return Optional.ofNullable(finished);
    }

}
