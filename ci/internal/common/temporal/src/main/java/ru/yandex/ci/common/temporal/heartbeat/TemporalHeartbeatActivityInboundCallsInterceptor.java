package ru.yandex.ci.common.temporal.heartbeat;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import io.temporal.activity.ActivityExecutionContext;
import io.temporal.common.interceptors.ActivityInboundCallsInterceptor;
import io.temporal.common.interceptors.ActivityInboundCallsInterceptorBase;

public class TemporalHeartbeatActivityInboundCallsInterceptor extends ActivityInboundCallsInterceptorBase {

    private final TemporalWorkerHeartbeatService heartbeatService;

    @Nullable
    private ActivityExecutionContext context;

    public TemporalHeartbeatActivityInboundCallsInterceptor(TemporalWorkerHeartbeatService heartbeatService,
                                                            ActivityInboundCallsInterceptor next) {
        super(next);
        this.heartbeatService = heartbeatService;
    }


    @Override
    public void init(ActivityExecutionContext context) {
        this.context = context;
        super.init(context);
    }

    @Override
    public ActivityOutput execute(ActivityInput input) {
        Preconditions.checkState(context != null);
        heartbeatService.registerActivity(context, Thread.currentThread());
        try {
            return super.execute(input);
        } finally {
            heartbeatService.unregisterActivity(context);
        }
    }
}
