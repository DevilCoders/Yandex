package ru.yandex.ci.flow.engine.runtime.test_data.common;

import java.util.UUID;
import java.util.concurrent.Semaphore;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTester;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

import static org.junit.jupiter.api.Assertions.fail;

/**
 * Джоба для тестирования отмены джоб. Предполагаемый сценарий использования:
 * 1. Тест запускает флоу с этой джобой в отдельном потоке ({@link FlowTester#runScheduledJobsToCompletionAsync()}).
 * 2. Тест вызывает semaphore.acquire() чтобы дождаться запуска этой джобы.
 * 3. Эта джоба запускается, вызывает semaphore.release() чтобы пропустить тест дальше и повисает надолго.
 * 4. Тест делает что-то, что должно привести к отмене этой джобы, и ждёт завершения всех джоб
 * (Thread#join на треде из п. 1).
 * 5. Если всё ок, то эта джоба отменяется. Если не ок, то эта джоба упадёт с AssertionError. Тест должен проверить
 * {@link JobState#getLastStatusChangeType()}
 * 6. Если что-то, что должно привести к отмене этой джобы, из п. 4 должно также привести к созданию второго запуска, то
 * этот второй запуск завершится мгновенно и успешно.
 *
 */
@Slf4j
@RequiredArgsConstructor
public class WaitingForInterruptOnceJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("7ac04b6f-d9a9-4ffe-b3c3-2d08de9716d7");

    private final Semaphore semaphore;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws InterruptedException {
        if (context.getJobState().getLaunches().size() > 1) {
            // Первый запуск ждёт отмену, последующие завершаются сразу
            return;
        }

        log.info("Working with semaphore: {}", semaphore);

        // Разрешаем тесту начинать рестартить первую джобу и тем самым отменять эту.
        semaphore.release();

        log.info("Semaphore released: {}", semaphore);

        // Ждём пока тест отменяет эту джобу.
        Thread.sleep(20000);

        // Если зашли сюда, то значит эта джоба не отменилась
        fail(getClass().getName() + " wasn't interrupted");
    }
}
