package ru.yandex.ci.storage.tms.cron;

import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.logbroker.event_producer.EmptyStorageEventsProducer;
import ru.yandex.ci.test.clock.OverridableClock;

import static org.assertj.core.api.Assertions.assertThat;

class IterationTimeoutCronTaskTest extends StorageYdbTestBase {
    @Test
    public void cancelsCheck() {
        var checkOne = CheckEntity.builder()
                .id(CheckEntity.Id.of(1L))
                .status(CheckStatus.RUNNING)
                .left(StorageRevision.EMPTY)
                .right(StorageRevision.EMPTY)
                .created(Instant.ofEpochSecond(1615293659))
                .build();

        var iterationOne = CheckIterationEntity.builder()
                .id(CheckIterationEntity.Id.of(checkOne.getId(), IterationType.FAST, 1))
                .created(checkOne.getCreated())
                .status(CheckStatus.RUNNING)
                .build();

        var checkTwo = CheckEntity.builder()
                .id(CheckEntity.Id.of(2L))
                .status(CheckStatus.RUNNING)
                .left(StorageRevision.EMPTY)
                .right(StorageRevision.EMPTY)
                .created(checkOne.getCreated().minus(1, ChronoUnit.DAYS))
                .build();

        var iterationTwo = CheckIterationEntity.builder()
                .id(CheckIterationEntity.Id.of(checkTwo.getId(), IterationType.FAST, 1))
                .created(checkTwo.getCreated())
                .status(CheckStatus.RUNNING)
                .build();

        this.db.currentOrTx(() -> {
            this.db.checks().save(checkOne);
            this.db.checkIterations().save(iterationOne);
            this.db.checks().save(checkTwo);
            this.db.checkIterations().save(iterationTwo);
        });

        var clock = new OverridableClock();
        clock.setTime(checkOne.getCreated());

        var task = new IterationTimeoutCronTask(
                db,
                new EmptyStorageEventsProducer(),
                clock,
                Duration.ofDays(1),
                Duration.ZERO
        );
        task.execute(null);

        this.db.readOnly().run(() -> {
            assertThat(this.db.checkIterations().get(iterationOne.getId()).getStatus())
                    .isEqualTo(CheckStatus.RUNNING);
            assertThat(this.db.checkIterations().get(iterationTwo.getId()).getStatus())
                    .isEqualTo(CheckStatus.CANCELLING_BY_TIMEOUT);
        });
    }
}
