package ru.yandex.ci.engine.flow;

import java.time.Duration;

import org.assertj.core.data.Percentage;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.http.HttpStatus;

import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxResponse;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.test.clock.OverridableClock;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@SuppressWarnings("unchecked")
@ExtendWith(MockitoExtension.class)
class SandboxTaskPollerTest {

    private final long taskId = 421;

    private final OverridableClock clock = new OverridableClock();

    private final SandboxTaskPollerSettings settings = new SandboxTaskPollerSettings(
            clock, Duration.ofMillis(1), Duration.ofMillis(30), Duration.ofMinutes(10),
            10, 5, 5);

    @Mock
    private SandboxClient client;

    @Mock
    private SandboxTaskPoller.TaskUpdateCallback callback;

    @Test
    void pollingInterval() {
        assertThat(sleepIntervalSeconds(21, 42, 0, 7)).isEqualTo(21);
        assertThat(sleepIntervalSeconds(2, 60, 0, 100)).isEqualTo(2);

        assertThat(sleepIntervalSeconds(2, 60, 10, 100))
                .isCloseTo(3, Percentage.withPercentage(10));

        assertThat(sleepIntervalSeconds(2, 60, 25, 100))
                .isCloseTo(5, Percentage.withPercentage(10));

        assertThat(sleepIntervalSeconds(2, 60, 50, 100))
                .isCloseTo(10, Percentage.withPercentage(10));

        assertThat(sleepIntervalSeconds(2, 60, 75, 100))
                .isCloseTo(25, Percentage.withPercentage(10));

        assertThat(sleepIntervalSeconds(2, 60, 90, 100))
                .isCloseTo(40, Percentage.withPercentage(10));

        assertThat(sleepIntervalSeconds(2, 60, 100, 100)).isEqualTo(60);

    }

    @Test
    void pollingInterval50PercentOnUnknownHeaders() {
        assertThat(sleepIntervalSeconds(2, 60, -1, -1))
                .isCloseTo(10, Percentage.withPercentage(10));
    }

    private int sleepIntervalSeconds(int minSeconds, int maxSeconds,
                                     int apiQuotaConsumptionMillis, int apiQuotaMillis) {
        int millis = SandboxTaskPoller.getSleepIntervalMillis(
                new SandboxTaskPollerSettings(
                        clock,
                        Duration.ofSeconds(minSeconds),
                        Duration.ofSeconds(maxSeconds),
                        Duration.ofMinutes(10),
                        0, 0, 0
                ),
                SandboxResponse.of(null, apiQuotaConsumptionMillis, apiQuotaMillis)
        );
        return (int) Math.round(millis / 1000.0);
    }

    @Test
    void simplePolling() throws Exception {

        when(client.getTaskAudit(taskId)).thenReturn(
                SandboxTestData.taskAudit(SandboxTaskStatus.ENQUEUING),
                SandboxTestData.taskAudit(SandboxTaskStatus.ENQUEUED),
                SandboxTestData.taskAudit(SandboxTaskStatus.ASSIGNED),
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskAudit(SandboxTaskStatus.FINISHING),
                SandboxTestData.taskAudit(SandboxTaskStatus.SUCCESS)
        );
        when(client.getTask(taskId)).thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS));

        when(callback.onStatusUpdate(any(SandboxTaskStatus.class))).thenReturn(true);
        when(callback.onStatusUpdate(SandboxTaskStatus.SUCCESS)).thenReturn(false);

        SandboxTaskPoller.pollTask(settings, client, taskId, false, callback);
    }

    @Test
    void notUseAuditEachNRequests() throws Exception {

        //Always return incorrect
        when(client.getTaskAudit(taskId)).thenReturn(
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING)
        );

        when(client.getTaskStatus(taskId)).thenReturn(
                SandboxTestData.taskStatus(SandboxTaskStatus.FINISHING),
                SandboxTestData.taskStatus(SandboxTaskStatus.FAILURE)
        );
        when(client.getTask(taskId)).thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.FAILURE));


        when(callback.onStatusUpdate(any(SandboxTaskStatus.class))).thenReturn(true);
        when(callback.onStatusUpdate(SandboxTaskStatus.FAILURE)).thenReturn(false);

        SandboxTaskPoller.pollTask(settings, client, taskId, false, callback);

        verify(client, times(18)).getTaskAudit(taskId);
        verify(client, times(2)).getTaskStatus(taskId);
        verify(client, times(1)).getTask(taskId);

        verify(callback).onStatusUpdate(SandboxTaskStatus.FINISHING);
        verify(callback).onStatusUpdate(SandboxTaskStatus.FAILURE);
    }

    @Test
    void updateTaskOutputEachNRequests() throws InterruptedException {

        SandboxTaskPollerSettings testSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(1), Duration.ofMillis(30), Duration.ofMinutes(10),
                5, 5, 5
        );

        when(client.getTaskAudit(taskId)).thenReturn(
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskAudit(SandboxTaskStatus.SUCCESS)
        );

        when(client.getTask(taskId)).thenReturn(
                SandboxTestData.taskOutput(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS)
        );

        when(callback.onStatusUpdate(any(SandboxTaskStatus.class))).thenReturn(true);
        when(callback.onStatusUpdate(SandboxTaskStatus.SUCCESS)).thenReturn(false);

        when(callback.onTaskOutputUpdate(any())).thenReturn(true);

        var output = SandboxTaskPoller.pollTask(testSettings, client, taskId, true, callback);

        verify(client, times(6)).getTaskAudit(taskId);
        verify(client, times(2)).getTask(taskId);

        verify(callback).onStatusUpdate(SandboxTaskStatus.EXECUTING);
        verify(callback).onTaskOutputUpdate(SandboxTestData.taskOutput(SandboxTaskStatus.EXECUTING).getData());
        verify(callback).onStatusUpdate(SandboxTaskStatus.SUCCESS);

        assertThat(output).isEqualTo(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS).getData());
    }

    @Test
    void useActualTaskOutput() throws InterruptedException {
        when(client.getTaskAudit(taskId)).thenReturn(SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING));

        when(client.getTask(taskId)).thenReturn(
                SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS)
        );
        when(callback.onStatusUpdate(any(SandboxTaskStatus.class))).thenReturn(true);
        when(callback.onTaskOutputUpdate(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS).getData()))
                .thenReturn(false);

        var output = SandboxTaskPoller.pollTask(settings, client, taskId, true, callback);

        verify(client, times(4)).getTaskAudit(taskId);
        verify(client, times(1)).getTask(taskId);
        assertThat(output).isEqualTo(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS).getData());
    }

    @Test
    void maxUnexpectedErrorsInRow() {

        SandboxTaskPollerSettings testSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(1), Duration.ofMillis(30), Duration.ofMinutes(10),
                10, 5, 5);

        when(client.getTaskAudit(taskId)).thenThrow(new HttpException("", 42, 500, ""));

        assertThatThrownBy(() -> SandboxTaskPoller.pollTask(testSettings, client, taskId, true, callback))
                .isInstanceOf(HttpException.class);

    }

    @Test
    void notEnoughUnexpectedErrorsInRow() throws InterruptedException {

        SandboxTaskPollerSettings testSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(1), Duration.ofMillis(30), Duration.ofMinutes(10),
                10, 5, 3
        );

        var ex = new HttpException("", 42, 500, "");
        when(client.getTaskAudit(taskId))
                .thenThrow(ex)
                .thenThrow(ex)
                .thenReturn(SandboxTestData.taskAudit(SandboxTaskStatus.SUCCESS));


        when(client.getTask(taskId)).thenReturn(
                SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS)
        );
        when(callback.onStatusUpdate(SandboxTaskStatus.SUCCESS)).thenReturn(false);

        var output = SandboxTaskPoller.pollTask(testSettings, client, taskId, false, callback);

        verify(client, times(3)).getTaskAudit(taskId);
        assertThat(output).isEqualTo(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS).getData());

    }

    @Test
    void quotaExceededAndRestored() throws InterruptedException {
        SandboxTaskPollerSettings testSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(1), Duration.ofMillis(1), Duration.ofSeconds(1000),
                100, 5, 1
        );

        clock.setEpoch();
        int return429UntilSeconds = 500;
        int stepSeconds = 100;
        when(client.getTaskAudit(taskId)).then(
                invocation -> {
                    long timeSeconds = clock.instant().getEpochSecond() + stepSeconds;
                    clock.setTimeSeconds(timeSeconds);
                    if (timeSeconds < return429UntilSeconds) {
                        throw new HttpException("", 42, HttpStatus.TOO_MANY_REQUESTS.value(), null);
                    }
                    return SandboxTestData.taskAudit(SandboxTaskStatus.SUCCESS);
                });

        when(client.getTask(taskId)).thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS));

        var output = SandboxTaskPoller.pollTask(testSettings, client, taskId, false, callback);

        verify(callback, times(1)).onQuotaExceeded();
        verify(callback, times(1)).onQuotaRestored();
        assertThat(output).isEqualTo(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS).getData());
    }

    @Test
    void quotaExceeded() {

        SandboxTaskPollerSettings testSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(1), Duration.ofMillis(1), Duration.ofSeconds(1000),
                100, 5, 1
        );

        clock.setEpoch();
        when(client.getTaskAudit(taskId)).then(
                invocation -> {
                    clock.plusSeconds(100);
                    throw new HttpException("", 42, HttpStatus.TOO_MANY_REQUESTS.value(), null);
                });

        assertThatThrownBy(() -> SandboxTaskPoller.pollTask(testSettings, client, taskId, false, callback))
                .hasMessageContaining("Quota exceeded for too long");

        verify(callback, times(1)).onQuotaExceeded();
        verify(client, times(12)).getTaskAudit(taskId);

        verify(client, never()).getTask(taskId);
    }


    @Test
    void quotaExceededAndRestoredAndExceededAgain() {
        SandboxTaskPollerSettings testSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(1), Duration.ofMillis(1), Duration.ofSeconds(1000),
                100, 5, 1
        );

        clock.setEpoch();
        int return429UntilSeconds = 500;
        int return429AfterSeconds = 1000;

        int stepSeconds = 100;
        when(client.getTaskAudit(taskId)).then(
                invocation -> {
                    long timeSeconds = clock.instant().getEpochSecond() + stepSeconds;
                    clock.setTimeSeconds(timeSeconds);
                    if (timeSeconds < return429UntilSeconds || timeSeconds > return429AfterSeconds) {
                        throw new HttpException("", 42, HttpStatus.TOO_MANY_REQUESTS.value(), null);
                    }
                    return SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING);
                });

        when(callback.onStatusUpdate(any())).thenReturn(true);

        assertThatThrownBy(() -> SandboxTaskPoller.pollTask(testSettings, client, taskId, false, callback))
                .hasMessageContaining("Quota exceeded for too long");

        verify(callback, times(2)).onQuotaExceeded();
        verify(callback, times(1)).onQuotaRestored();
    }

    @Timeout(5) // Execute longer than 5 seconds? Must be an endless cycle
    @Test
    void continueExecutionOnListenerFailureAfterAudit() throws InterruptedException {
        when(client.getTaskAudit(taskId)).thenReturn(
                SandboxTestData.taskAudit(SandboxTaskStatus.ENQUEUING),
                SandboxTestData.taskAudit(SandboxTaskStatus.ENQUEUED),
                SandboxTestData.taskAudit(SandboxTaskStatus.ASSIGNED),
                SandboxTestData.taskAudit(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskAudit(SandboxTaskStatus.FINISHING),
                SandboxTestData.taskAudit(SandboxTaskStatus.SUCCESS),
                SandboxTestData.taskAudit(SandboxTaskStatus.SUCCESS)
        );
        when(client.getTask(taskId)).thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS));

        when(callback.onStatusUpdate(any(SandboxTaskStatus.class))).thenReturn(true);
        when(callback.onStatusUpdate(SandboxTaskStatus.SUCCESS))
                .thenThrow(new RuntimeException("Unexpected")) // Must be repeated proper
                .thenReturn(false);

        SandboxTaskPoller.pollTask(settings, client, taskId, false, callback);
    }

    @Timeout(5) // Execute longer than 5 seconds? Must be an endless cycle
    @Test
    void continueExecutionOnListenerFailureAfterTask() throws InterruptedException {
        var successOutput = SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS);
        when(client.getTask(taskId))
                .thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.ENQUEUING))
                .thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.ENQUEUED))
                .thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.ASSIGNED))
                .thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.EXECUTING))
                .thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.FINISHING))
                .thenReturn(successOutput)
                .thenReturn(successOutput);

        when(callback.onTaskOutputUpdate(any(SandboxTaskOutput.class)))
                .thenReturn(true);
        when(callback.onTaskOutputUpdate(eq(successOutput.getData())))
                .thenThrow(new RuntimeException("Unexpected")) // Must be repeated proper
                .thenReturn(false);

        SandboxTaskPollerSettings testSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(1), Duration.ofMillis(30), Duration.ofMinutes(10),
                5, 1, 5
        );
        SandboxTaskPoller.pollTask(testSettings, client, taskId, true, callback);
    }


}

