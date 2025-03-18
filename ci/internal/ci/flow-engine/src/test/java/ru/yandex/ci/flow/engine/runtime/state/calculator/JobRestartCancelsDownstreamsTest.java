package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.concurrent.Semaphore;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.common.WaitingForInterruptOnceJob;

import static org.junit.jupiter.api.Assertions.assertEquals;

@Slf4j
public class JobRestartCancelsDownstreamsTest extends FlowEngineTestBase {

    @Autowired
    private FlowStateService flowStateService;

    @Autowired
    private Semaphore semaphore;

    /**
     * Если этот тест упал по таймауту, то скорее всего не отменилась джоба, которая должна была отмениться.
     */
    @Timeout(30)
    @Test
    public void shouldInterruptDownstreams() throws InterruptedException {
        String triggeredBy = "some user";

        // Создаём флоу job1 -> job2
        FlowBuilder builder = FlowBuilder.create();
        JobBuilder job1 = builder.withJob(DummyJob.ID, "job1");
        JobBuilder job2 = builder.withJob(WaitingForInterruptOnceJob.ID, "job2").withUpstreams(job1);

        // Запускаем флоу в отдельном потоке чтобы иметь возможность что-то делать пока он работает.
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(builder.build());
        Thread thread = flowTester.runScheduledJobsToCompletionAsync();

        // Ждём запуска job2. Первый запуск job2 вызывает semaphore.release() и долго висит после этого.
        semaphore.acquire();

        // Пока job2 работает, рестартим job1. Это отменит первый запуск job2 и поставит в очередь второй запуск
        // job1.
        flowStateService.recalc(flowLaunchId, new TriggerEvent(job1.getId(), triggeredBy, false));

        log.info("Waiting to interrupt...");
        // Ждём завершения runFlowToCompletionAsync.
        thread.join();

        // После всех этих махинаций:
        // - у job1 два запуска, оба успешные
        // - у job2 два запуска, первый отменён пользователем triggeredBy, второй успешный
        FlowLaunchEntity flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
        JobState job1State = flowLaunch.getJobState(job1.getId());
        assertEquals(2, job1State.getLaunches().size());
        assertEquals(StatusChangeType.SUCCESSFUL, job1State.getLaunches().get(0).getLastStatusChangeType());
        assertEquals(StatusChangeType.SUCCESSFUL, job1State.getLaunches().get(1).getLastStatusChangeType());

        JobState job2State = flowLaunch.getJobState(job2.getId());
        assertEquals(2, job2State.getLaunches().size());
        assertEquals(StatusChangeType.INTERRUPTED, job2State.getLaunches().get(0).getLastStatusChangeType());
        assertEquals(triggeredBy, job2State.getLaunches().get(0).getInterruptedBy());
        assertEquals(StatusChangeType.SUCCESSFUL, job2State.getLaunches().get(1).getLastStatusChangeType());
    }
}
