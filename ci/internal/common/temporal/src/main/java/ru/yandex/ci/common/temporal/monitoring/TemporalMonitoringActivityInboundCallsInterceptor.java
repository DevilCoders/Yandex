package ru.yandex.ci.common.temporal.monitoring;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import io.temporal.activity.ActivityExecutionContext;
import io.temporal.activity.ActivityInfo;
import io.temporal.common.interceptors.ActivityInboundCallsInterceptor;
import io.temporal.common.interceptors.ActivityInboundCallsInterceptorBase;

@SuppressWarnings("checkstyle:ParameterName")
public class TemporalMonitoringActivityInboundCallsInterceptor extends ActivityInboundCallsInterceptorBase {

    private final TemporalMonitoringService monitoringService;

    @Nullable
    private ActivityInfo activityInfo;


    public TemporalMonitoringActivityInboundCallsInterceptor(TemporalMonitoringService monitoringService,
                                                             ActivityInboundCallsInterceptor next) {
        super(next);
        this.monitoringService = monitoringService;
    }

    @Override
    public void init(ActivityExecutionContext context) {
        this.activityInfo = context.getInfo();
        try {
            super.init(context);
        } catch (Exception e) {
            Preconditions.checkState(activityInfo != null);
            monitoringService.notifyActivityFailed(activityInfo, e);
        }
    }

    @Override
    public ActivityOutput execute(ActivityInput input) {
        Preconditions.checkState(activityInfo != null);
        try {
            ActivityOutput output = super.execute(input);
            monitoringService.notifyActivitySuccess(activityInfo);
            return output;
        } catch (Exception e) {
            monitoringService.notifyActivityFailed(activityInfo, e);
            throw e;
        }
    }
}
