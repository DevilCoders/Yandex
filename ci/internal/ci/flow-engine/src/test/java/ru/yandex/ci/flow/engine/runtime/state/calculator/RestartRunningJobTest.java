package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.concurrent.Semaphore;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.common.WaitingForInterruptOnceJob;

import static org.assertj.core.api.Assertions.assertThat;

@Slf4j
public class RestartRunningJobTest extends FlowEngineTestBase {
    @Autowired
    private FlowStateService flowStateService;

    @Autowired
    private Semaphore semaphore;

    /**
     * Если этот тест упал по таймауту, то скорее всего не отменилась джоба, которая должна была отмениться.
     */
    @Timeout(30)
    @Test
    public void test() throws InterruptedException {
        String triggeredBy = "some user";

        // Создаём флоу с одной джобой
        FlowBuilder builder = FlowBuilder.create();
        JobBuilder job1 = builder.withJob(WaitingForInterruptOnceJob.ID, "job1");

        // Запускаем флоу в отдельном потоке чтобы иметь возможность что-то делать пока он работает.
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(builder.build());
        Thread thread = flowTester.runScheduledJobsToCompletionAsync();

        log.info("Waiting for semaphore: {}", semaphore);

        // Ждём запуска job1. Первый запуск job1 вызывает semaphore.release() и долго висит после этого.
        semaphore.acquire();

        log.info("Semaphore acquired: {}", semaphore);

        // Пока job1 работает, триггерим её с shouldRestartIfAlreadyRunning=true.
        // Это отменит первый запуск job1 и поставит в очередь второй запуск job1.
        flowStateService.recalc(flowLaunchId, new TriggerEvent(job1.getId(), triggeredBy, true));

        log.info("Waiting to interrupt...");
        // Ждём завершения runFlowToCompletionAsync.
        thread.join();

        FlowLaunchEntity flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
        assertThat(flowLaunch.getJobState(job1.getId()).getLaunches())
                .extracting(JobLaunch::getLastStatusChangeType)
                .containsExactly(StatusChangeType.INTERRUPTED, StatusChangeType.SUCCESSFUL);
    }
}
