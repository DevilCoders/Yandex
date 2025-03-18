package ru.yandex.ci.engine.launch.auto;

import java.time.Duration;

import lombok.Value;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.commune.bazinga.scheduler.ActiveUidBehavior;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDropType;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDuplicateBehavior;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleConstant;
import ru.yandex.commune.bazinga.scheduler.schedule.ReschedulePolicy;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

public class DelayedAutoReleaseLaunchTask extends AbstractOnetimeTask<DelayedAutoReleaseLaunchTask.Params> {
    private final AutoReleaseService autoReleaseService;

    public DelayedAutoReleaseLaunchTask(AutoReleaseService autoReleaseService) {
        super(DelayedAutoReleaseLaunchTask.Params.class);
        this.autoReleaseService = autoReleaseService;
    }

    public DelayedAutoReleaseLaunchTask(CiProcessId processId) {
        super(new Params(processId.asString()));
        this.autoReleaseService = null;
    }

    @Override
    protected void execute(Params params, ExecutionContext context) throws Exception {
        autoReleaseService.scheduleLaunchAfterScheduledTimeHasCome(params.getProcessId());
    }

    @Override
    public ActiveUidBehavior activeUidBehavior() {
        return new ActiveUidBehavior(ActiveUidDropType.WHEN_FINISHED, ActiveUidDuplicateBehavior.MERGE);
    }

    @Override
    public ReschedulePolicy reschedulePolicy() {
        var timeout = org.joda.time.Duration.standardSeconds(3);
        return new RescheduleConstant(timeout, 3);
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        String ciProcessId;

        public CiProcessId getProcessId() {
            return CiProcessId.ofString(ciProcessId);
        }
    }
}
