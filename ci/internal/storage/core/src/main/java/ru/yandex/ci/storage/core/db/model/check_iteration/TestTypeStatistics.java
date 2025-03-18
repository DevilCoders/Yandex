package ru.yandex.ci.storage.core.db.model.check_iteration;

import java.time.Instant;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.utils.MathUtils;
import ru.yandex.ci.ydb.Persisted;

import static ru.yandex.ci.storage.core.Common.TestTypeStatus.TTS_COMPLETED;
import static ru.yandex.ci.storage.core.Common.TestTypeStatus.TTS_NOT_STARTED;
import static ru.yandex.ci.storage.core.Common.TestTypeStatus.TTS_RUNNING;
import static ru.yandex.ci.storage.core.Common.TestTypeStatus.TTS_WAITING_FOR_CHUNKS;
import static ru.yandex.ci.storage.core.Common.TestTypeStatus.TTS_WAITING_FOR_CONFIGURE;

@Persisted
@Value
@Builder(toBuilder = true)
public class TestTypeStatistics {
    public static final TestTypeStatistics EMPTY = TestTypeStatistics.builder().build();

    @Nullable
    Statistics configure;

    @Nullable
    Statistics build;

    @Nullable
    Statistics style;

    @Nullable
    Statistics smallTests;

    @Nullable
    Statistics mediumTests;

    @Nullable
    Statistics largeTests;

    @Nullable
    Statistics teTests;

    @Nullable
    Statistics nativeBuilds;

    public Statistics getConfigure() {
        return configure == null ? Statistics.EMPTY : configure;
    }

    public Statistics getBuild() {
        return build == null ? Statistics.EMPTY : build;
    }

    public Statistics getStyle() {
        return style == null ? Statistics.EMPTY : style;
    }

    public Statistics getSmallTests() {
        return smallTests == null ? Statistics.EMPTY : smallTests;
    }

    public Statistics getMediumTests() {
        return mediumTests == null ? Statistics.EMPTY : mediumTests;
    }

    public Statistics getLargeTests() {
        return largeTests == null ? Statistics.EMPTY : largeTests;
    }

    public Statistics getTeTests() {
        return teTests == null ? Statistics.EMPTY : teTests;
    }

    public Statistics getNativeBuilds() {
        return nativeBuilds == null ? Statistics.EMPTY : nativeBuilds;
    }

    public Statistics get(Common.ChunkType chunkType) {
        return switch (chunkType) {
            case CT_CONFIGURE -> getConfigure();
            case CT_BUILD -> getBuild();
            case CT_STYLE -> getStyle();
            case CT_SMALL_TEST -> getSmallTests();
            case CT_MEDIUM_TEST -> getMediumTests();
            case CT_LARGE_TEST -> getLargeTests();
            case CT_TESTENV -> getTeTests();
            case CT_NATIVE_BUILD -> getNativeBuilds();
            case UNRECOGNIZED -> throw new RuntimeException("Unrecognized");
        };
    }

    public TestTypeStatistics onCompleted(Common.ChunkType chunkType, boolean canFinish) {
        return set(
                this.toBuilder(),
                chunkType,
                this.get(chunkType).onCompleted(canFinish)
        ).build();
    }

    public TestTypeStatistics onChunkCompleted(ChunkEntity.Id chunkId) {
        return set(
                this.toBuilder(),
                chunkId.getChunkType(),
                this.get(chunkId.getChunkType()).onChunkCompleted(chunkId)
        ).build();
    }

    public int calculateProgress() {
        var completed = 0;
        var registered = 0;

        for (var chunkType : Common.ChunkType.values()) {
            if (chunkType.equals(Common.ChunkType.UNRECOGNIZED)) {
                continue;
            }

            var category = get(chunkType);
            completed += category.completedTasks;
            registered += category.registeredTasks;
        }


        return (int) Math.round(MathUtils.percent(completed, registered));
    }

    public TestTypeStatistics resetCompleted() {
        return new TestTypeStatistics(
                configure == null ? null : configure.resetCompleted(),
                build == null ? null : build.resetCompleted(),
                style == null ? null : style.resetCompleted(),
                smallTests == null ? null : smallTests.resetCompleted(),
                mediumTests == null ? null : mediumTests.resetCompleted(),
                largeTests == null ? null : largeTests.resetCompleted(),
                teTests == null ? null : teTests.resetCompleted(),
                nativeBuilds == null ? null : nativeBuilds.resetCompleted()
        );
    }

    public Set<Common.ChunkType> getWaitingForConfigure() {
        return getInStatus(TTS_WAITING_FOR_CONFIGURE);
    }

    public Set<Common.ChunkType> getRunning() {
        return getInStatus(Common.TestTypeStatus.TTS_RUNNING);
    }

    private Set<Common.ChunkType> getInStatus(Common.TestTypeStatus status) {
        var result = new HashSet<Common.ChunkType>(6);
        for (var chunkType : Common.ChunkType.values()) {
            if (chunkType.equals(Common.ChunkType.UNRECOGNIZED)) {
                continue;
            }

            if (this.get(chunkType).getStatus().equals(status)) {
                result.add(chunkType);
            }
        }

        return result;
    }

    public TestTypeStatistics onChunkFinalizationStarted(Common.ChunkType chunkType, Set<ChunkEntity.Id> chunks) {
        var builder = this.toBuilder();

        set(builder, chunkType, this.get(chunkType).onChunkFinalizationStarted(chunks));

        return builder.build();
    }

    public String printStatus() {
        return "C:%s, B:%s, Y:%s, S:%s, M:%s, L:%s, T:%s".formatted(
                getConfigure().printStatus(),
                getBuild().printStatus(),
                getStyle().printStatus(),
                getSmallTests().printStatus(),
                getMediumTests().printStatus(),
                getLargeTests().printStatus(),
                getTeTests().printStatus()
        );
    }

    private static Builder set(Builder builder, Common.ChunkType chunkType, Statistics statistics) {
        switch (chunkType) {
            case CT_CONFIGURE -> builder.configure(statistics);
            case CT_BUILD -> builder.build(statistics);
            case CT_STYLE -> builder.style(statistics);
            case CT_SMALL_TEST -> builder.smallTests(statistics);
            case CT_MEDIUM_TEST -> builder.mediumTests(statistics);
            case CT_LARGE_TEST -> builder.largeTests(statistics);
            case CT_TESTENV -> builder.teTests(statistics);
            case CT_NATIVE_BUILD -> builder.nativeBuilds(statistics);
            default -> throw new RuntimeException("Unrecognized");
        }

        return builder;
    }

    public Set<Common.ChunkType> getNotCompleted() {
        var result = new HashSet<Common.ChunkType>(6);
        for (var chunkType : Common.ChunkType.values()) {
            if (chunkType.equals(Common.ChunkType.UNRECOGNIZED)) {
                continue;
            }

            var status = this.get(chunkType).getStatus();
            if (status != TTS_COMPLETED && status != TTS_NOT_STARTED) {
                result.add(chunkType);
            }
        }

        return result;
    }

    public TestTypeStatistics onRegistered(Common.CheckTaskType type) {
        var builder = this.toBuilder();
        switch (type) {
            case CTT_AUTOCHECK, UNRECOGNIZED -> {
                builder.configure(this.getConfigure().onRegistered());
                builder.build(this.getBuild().onRegistered());
                builder.style(this.getStyle().onRegistered());
                builder.smallTests(this.getSmallTests().onRegistered());
                builder.mediumTests(this.getMediumTests().onRegistered());
                builder.largeTests(this.getLargeTests().onRegistered());
            }
            case CTT_LARGE_TEST -> builder.largeTests(this.getLargeTests().onRegistered());
            case CTT_TESTENV -> builder.teTests(this.getTeTests().onRegistered());
            case CTT_NATIVE_BUILD -> builder.nativeBuilds(this.getNativeBuilds().onRegistered());
            default -> throw new IllegalStateException("Unexpected value: " + type);
        }

        return builder.build();
    }

    @Persisted
    @Value
    @lombok.Builder(toBuilder = true)
    public static class Statistics {
        public static final Statistics EMPTY = Statistics.builder()
                .status(TTS_NOT_STARTED)
                .finishingChunks(Set.of())
                .build();

        @Nullable
        Common.TestTypeStatus status;

        int registeredTasks;
        int completedTasks;
        double completedPercent;

        @Nullable
        Instant completed;

        @Nullable
        Instant finalized;

        @Nullable // used only on TTS_WAITING_FOR_CHUNKS state.
        Set<ChunkEntity.Id> finishingChunks;

        public Statistics onRegistered() {
            if (isCompleted()) {
                return this;
            }

            return this.toBuilder()
                    .registeredTasks(registeredTasks + 1)
                    .status(getStatus() == TTS_NOT_STARTED ? TTS_RUNNING : status)
                    .completedPercent(MathUtils.percent(completedTasks, registeredTasks + 1))
                    .build();
        }

        private Statistics onCompleted(boolean canFinish) {
            if (isCompleted()) {
                return this;
            }

            var updatedCompleted = completedTasks + 1;

            if (updatedCompleted == this.registeredTasks) {
                if (canFinish) {
                    return this.toBuilder()
                            .status(TTS_WAITING_FOR_CONFIGURE)
                            .completedTasks(updatedCompleted)
                            .completedPercent(100)
                            .completed(Instant.now())
                            .build();
                }
            }

            return this.toBuilder()
                    .completedTasks(updatedCompleted)
                    .completedPercent(MathUtils.percent(updatedCompleted, this.registeredTasks))
                    .build();
        }

        public Statistics onChunkCompleted(ChunkEntity.Id chunkId) {
            if (!this.isWaitingForChunks()) {
                return this;
            }

            var updatedChunks = new HashSet<>(this.getFinishingChunks());
            updatedChunks.remove(chunkId);

            return this.toBuilder()
                    .finishingChunks(updatedChunks)
                    .status(
                            updatedChunks.isEmpty() ?
                                    TTS_COMPLETED : TTS_WAITING_FOR_CHUNKS
                    )
                    .finalized(updatedChunks.isEmpty() ? Instant.now() : null)
                    .build();
        }

        public boolean isCompleted() {
            return getStatus().equals(TTS_COMPLETED);
        }

        public boolean isWaitingForConfigure() {
            return getStatus().equals(TTS_WAITING_FOR_CONFIGURE);
        }

        public boolean isRunning() {
            return getStatus().equals(TTS_RUNNING);
        }

        public boolean hasStarted() {
            return !getStatus().equals(TTS_NOT_STARTED);
        }

        public boolean isWaitingForChunks() {
            return getStatus().equals(TTS_WAITING_FOR_CHUNKS);
        }

        public Statistics resetCompleted() {
            return new Statistics(TTS_RUNNING, registeredTasks, completedTasks, completedPercent, null, null, Set.of());
        }

        public Common.TestTypeStatus getStatus() {
            return status == null ? TTS_NOT_STARTED : status;
        }

        public Set<ChunkEntity.Id> getFinishingChunks() {
            return finishingChunks == null ? Set.of() : finishingChunks;
        }

        public Statistics onChunkFinalizationStarted(Set<ChunkEntity.Id> ids) {
            if (!isWaitingForConfigure()) {
                return this;
            }

            if (ids.isEmpty() && this.getFinishingChunks().isEmpty()) {
                return this.toBuilder()
                        .status(TTS_COMPLETED)
                        .finalized(Instant.now())
                        .build();
            }

            var chunkIds = new HashSet<>(this.getFinishingChunks());
            chunkIds.addAll(ids);

            return this.toBuilder()
                    .finishingChunks(chunkIds)
                    .status(TTS_WAITING_FOR_CHUNKS)
                    .build();
        }

        public String printStatus() {
            if (this.getStatus().equals(TTS_WAITING_FOR_CHUNKS)) {
                return this.getStatus().name() + " (chunks: [%s])".formatted(
                        this.getFinishingChunks().stream()
                                .map(x -> x.getNumber().toString())
                                .collect(Collectors.joining(", "))
                );
            }

            return this.getStatus().name();
        }
    }
}
