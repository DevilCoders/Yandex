package ru.yandex.ci.common.bazinga;

import java.util.concurrent.TimeUnit;

import org.joda.time.Duration;

import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.OnetimeTaskSupport;
import ru.yandex.commune.bazinga.scheduler.schedule.CompoundReschedulePolicy;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleConstant;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleLinear;
import ru.yandex.commune.bazinga.scheduler.schedule.ReschedulePolicy;

public abstract class AbstractOnetimeTask<T> extends OnetimeTaskSupport<T> {

    private static final int MAX_RESCHEDULE_DELAY_SECONDS = (int) TimeUnit.HOURS.toSeconds(1);

    protected AbstractOnetimeTask(T params) {
        super(params);
    }

    protected AbstractOnetimeTask(Class<T> paramsClass) {
        super(paramsClass);
    }

    //Just for renaming parameters to params then implementing
    @Override
    protected abstract void execute(T params, ExecutionContext context) throws Exception;


    @Override
    public ReschedulePolicy reschedulePolicy() {
        return new CompoundReschedulePolicy(
                // Решедулим c увеличением по секунде, но не больше часа
                new RescheduleLinear(Duration.standardSeconds(1), MAX_RESCHEDULE_DELAY_SECONDS),
                // Решедулим бесконечно
                new RescheduleConstant(Duration.standardSeconds(MAX_RESCHEDULE_DELAY_SECONDS), Integer.MAX_VALUE)
        );
    }

    @Override
    public int priority() {
        return 0;
    }

    @Override
    public final Duration timeout() {
        return Duration.millis(getTimeout().toMillis());
    }

    public abstract java.time.Duration getTimeout();
}
