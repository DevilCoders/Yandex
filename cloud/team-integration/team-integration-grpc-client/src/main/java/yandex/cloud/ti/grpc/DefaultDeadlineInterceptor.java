package yandex.cloud.ti.grpc;

import java.time.Duration;
import java.util.concurrent.TimeUnit;

import io.grpc.CallOptions;
import io.grpc.Channel;
import io.grpc.ClientCall;
import io.grpc.ClientInterceptor;
import io.grpc.MethodDescriptor;
import org.jetbrains.annotations.NotNull;

public class DefaultDeadlineInterceptor implements ClientInterceptor {

    private final @NotNull Duration defaultDeadline;


    public DefaultDeadlineInterceptor(@NotNull Duration defaultDeadline) {
        this.defaultDeadline = defaultDeadline;
    }


    @Override
    public <ReqT, RespT> ClientCall<ReqT, RespT> interceptCall(
            MethodDescriptor<ReqT, RespT> method,
            CallOptions callOptions,
            Channel next
    ) {
        if (callOptions.getDeadline() == null) {
            callOptions = callOptions.withDeadlineAfter(defaultDeadline.toMillis(), TimeUnit.MILLISECONDS);
        }
        return next.newCall(method, callOptions);
    }

}
