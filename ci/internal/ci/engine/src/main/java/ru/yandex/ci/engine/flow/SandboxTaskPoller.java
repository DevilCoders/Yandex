package ru.yandex.ci.engine.flow;

import java.time.Instant;
import java.util.Objects;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;
import org.springframework.http.HttpStatus;

import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxResponse;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.TaskAuditRecord;

@Slf4j
@RequiredArgsConstructor(access = AccessLevel.PRIVATE)
public class SandboxTaskPoller {
    private final SandboxTaskPollerSettings settings;
    private final SandboxClient sandboxClient;
    private final long taskId;
    private final boolean updateTaskOutput;
    private final TaskUpdateCallback callback;

    @Nullable
    private Instant quotaExceededTime = null;

    private int requestNumber = 1;

    @Nullable
    private SandboxTaskStatus lastStatus = null;

    @Nullable
    private SandboxTaskOutput lastOutput = null;
    private boolean hasActualOutput = false;

    @Nullable
    private SandboxResponse<?> lastSandboxResponse = null;

    private boolean active = true;

    private int errorsInRow = 0;

    public static SandboxTaskOutput pollTask(
            SandboxTaskPollerSettings settings,
            SandboxClient sandboxClient,
            long taskId,
            boolean updateTaskOutput,
            TaskUpdateCallback callback
    ) throws InterruptedException {
        var poller = new SandboxTaskPoller(settings, sandboxClient, taskId, updateTaskOutput, callback);
        return poller.pollTask();
    }

    //TODO tests
    public SandboxTaskOutput pollTask() throws InterruptedException {
        log.info("Starting polling for task {}", taskId);
        while (active) {
            executeRequest(false);
            if (active) {
                sleep();
            }
        }
        log.info("Polling of task {} finished.", taskId);

        while (!hasActualOutput) {
            log.info("Requesting full task output");
            executeRequest(true);
        }
        Preconditions.checkState(lastOutput != null);
        return lastOutput;
    }

    private void executeRequest(boolean forceOutputUpdate) {
        try {
            if (forceOutputUpdate || shouldUpdateTaskOutput()) {
                updateTaskOutput();
            } else {
                updateTaskStatus();
            }
            handleSuccessRequest();
        } catch (HttpException e) {
            if (e.getHttpCode() == HttpStatus.TOO_MANY_REQUESTS.value()) {
                handleQuotaExceeded(e);
                return;
            }
            handleUnexpectedError(e);
        } catch (RuntimeException e) {
            handleUnexpectedError(e);
        }
    }

    private void handleUnexpectedError(RuntimeException e) {
        errorsInRow++;
        if (errorsInRow > settings.getMaxUnexpectedErrorsInRow()) {
            log.error("Too many errors in row {}, max allows {}.", errorsInRow, settings.getMaxUnexpectedErrorsInRow());
            throw e;
        }
        log.warn(
                "Got error. Skipping cause max errors in row not exceeded. Error number {}, max allowed {}",
                errorsInRow, settings.getMaxUnexpectedErrorsInRow(),
                e
        );
    }

    private void handleSuccessRequest() {
        requestNumber++;
        errorsInRow = 0;
        handleQuotaRestored();
    }

    private void handleQuotaRestored() {
        if (quotaExceededTime == null) {
            return;
        }
        log.info("Quota restored. Exceeded at {}", quotaExceededTime);
        callback.onQuotaRestored();
        quotaExceededTime = null;
    }

    private void handleQuotaExceeded(HttpException e) {
        Instant now = settings.getClock().instant();
        if (quotaExceededTime == null) {
            log.warn("Got first quota exceeded at {}", now);
            quotaExceededTime = now;
            callback.onQuotaExceeded();
            return;
        }

        if (now.isAfter(quotaExceededTime.plus(settings.getMaxWaitOnQuotaExceeded()))) {
            String errorMessage = "Quota exceeded for too long. " + quotaErrorString();
            log.error(errorMessage);
            throw new RuntimeException(errorMessage, e);
        }

        log.warn(
                "Quota still exceeded. " + quotaErrorString(),
                quotaExceededTime, settings.getMaxWaitOnQuotaExceeded()
        );
    }

    private String quotaErrorString() {
        return String.format(
                "First quota error at %s. Max wait time %s",
                quotaExceededTime, settings.getMaxWaitOnQuotaExceeded()
        );
    }

    private boolean shouldUpdateTaskOutput() {
        if (!updateTaskOutput) {
            return false;
        }
        return requestNumber % settings.getUpdateTaskOutputEachNRequests() == 0;
    }


    private void updateTaskOutput() {
        var taskOutputResponse = sandboxClient.getTask(taskId);
        var taskOutput = taskOutputResponse.getData();
        lastSandboxResponse = taskOutputResponse;
        if (!taskOutput.equals(lastOutput)) {
            log.info("TaskOutput updated");
            if (active) {
                active = callback.onTaskOutputUpdate(taskOutput);
            }
            lastOutput = taskOutput;
            lastStatus = taskOutput.getStatus();
        }
        hasActualOutput = true;
    }

    private void updateTaskStatus() {
        SandboxTaskStatus status = shouldUpdateTaskStatusViaAudit() ? getTaskStatusViaAudit() : getTaskStatus();
        if (!status.equals(lastStatus)) {
            log.info("Task status updated. {} -> {}", lastStatus, status);
            active = callback.onStatusUpdate(status);
            lastStatus = status;
        }
        hasActualOutput = false;
    }

    private boolean shouldUpdateTaskStatusViaAudit() {
        return requestNumber % settings.getNotUseAuditEachNRequests() != 0;
    }

    private SandboxTaskStatus getTaskStatusViaAudit() {
        var response = sandboxClient.getTaskAudit(taskId);
        lastSandboxResponse = response;

        return StreamEx.ofReversed(response.getData())
                .map(TaskAuditRecord::getStatus)
                .filter(Objects::nonNull)
                .findFirst()
                .orElseThrow();
    }

    private SandboxTaskStatus getTaskStatus() {
        var response = sandboxClient.getTaskStatus(taskId);
        lastSandboxResponse = response;
        return response.getData();
    }


    private void sleep() throws InterruptedException {
        Thread.sleep(getSleepIntervalMillis());
    }

    long getSleepIntervalMillis() {
        if (quotaExceededTime != null) {
            log.warn("Sandbox quota exceeded. Using maxPoolingInterval {}", settings.getMaxPollingInterval());
            return settings.getMaxPollingInterval().toMillis();
        }
        if (lastSandboxResponse == null) {
            log.warn("No last sandbox response. Using minPoolingInterval {}", settings.getMinPollingInterval());
            return settings.getMinPollingInterval().toMillis();
        }
        return getSleepIntervalMillis(settings, lastSandboxResponse);
    }

    @VisibleForTesting
    static int getSleepIntervalMillis(SandboxTaskPollerSettings settings, SandboxResponse<?> sandboxResponse) {
        if (settings.getMinPollingInterval().toMillis() == 0) {
            return 0;
        }
        int quotaConsumptionPercent = sandboxResponse.getApiQuotaConsumptionPercent();
        if (quotaConsumptionPercent < 0) {
            log.warn("Api quota consumption is unknown. Using median value (50%)");
            quotaConsumptionPercent = 50;
        }
        double pow = 1 + (settings.getWaitScalePow() - 1) * quotaConsumptionPercent * 0.01;
        int sleepIntervalMillis = (int) Math.pow((double) settings.getMinPollingInterval().toMillis(), pow);
        log.info(
                "Sandbox api quota consumption: {}%, sleep interval: {} ms",
                quotaConsumptionPercent,
                sleepIntervalMillis
        );
        return sleepIntervalMillis;
    }


    public interface TaskUpdateCallback {
        /**
         * @return true in should continue polling, false otherwise
         */
        boolean onStatusUpdate(SandboxTaskStatus status);

        /**
         * @return true in should continue polling, false otherwise
         */
        boolean onTaskOutputUpdate(SandboxTaskOutput taskOutput);

        void onQuotaExceeded();

        void onQuotaRestored();
    }


}
