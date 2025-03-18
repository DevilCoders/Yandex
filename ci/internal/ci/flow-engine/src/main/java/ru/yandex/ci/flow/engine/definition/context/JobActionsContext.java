package ru.yandex.ci.flow.engine.definition.context;

import java.util.Collections;
import java.util.List;
import java.util.function.BiConsumer;

import ru.yandex.ci.flow.engine.definition.context.impl.SupportType;

public interface JobActionsContext {
    void delayJobStart(long delayMillis, BiConsumer<JobContext, Long> remainingDelayCallback)
        throws InterruptedException;

    default void failJobIf(boolean shouldFail, String reason) {
        failJobIf(shouldFail, reason, SupportType.NONE);
    }

    default void failJobIf(boolean shouldFail, String reason, SupportType supportType) {
        if (shouldFail) {
            failJob(reason, Collections.singletonList(supportType));
        }
    }

    default void failJobIfNot(boolean shouldFail, String reason) {
        failJobIf(!shouldFail, reason);
    }

    default void failJobIfNot(boolean shouldFail, String reason, SupportType supportType) {
        failJobIf(!shouldFail, reason, supportType);
    }

    default void failJob(String reason, SupportType supportInfo) {
        failJob(reason, Collections.singletonList(supportInfo));
    }

    /**
     * Immediately fails job.
     *
     * @param reason job fail reason.
     */
    void failJob(String reason, List<SupportType> supportInfo);

    default void failJob(String reason, SupportType supportInfo, Throwable cause) {
        failJob(reason, Collections.singletonList(supportInfo), cause);
    }

    /**
     * Immediately fails job.
     *
     * @param reason job fail reason.
     * @param cause  job fail cause.
     */
    void failJob(String reason, List<SupportType> supportInfo, Throwable cause);
}
