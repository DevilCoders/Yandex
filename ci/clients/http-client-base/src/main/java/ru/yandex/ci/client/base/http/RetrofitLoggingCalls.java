package ru.yandex.ci.client.base.http;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import okhttp3.Request;
import org.apache.commons.lang3.StringUtils;
import org.asynchttpclient.extras.retrofit.AsyncHttpClientCall;
import org.asynchttpclient.filter.FilterContext;
import org.asynchttpclient.filter.RequestFilter;
import retrofit2.Call;
import retrofit2.Response;

@Slf4j
public class RetrofitLoggingCalls implements CallsMonitor<Call<?>, Response<?>> {
    private static final int FILTER_BODY_SIZE_LENGTH = 64 * 1024;

    private static final String LOG_REQUEST_HEADERS = "x-log-request-headers";
    private static final String LOG_REQUEST_BODY = "x-log-request-body";


    private static final int NO_RESPONSE = -1;
    private static final int ERROR_REQUEST = -2;
    private final String clientName;
    private final CallListener callListener;

    private RetrofitLoggingCalls(String clientName, CallListener callListener) {
        this.clientName = clientName;
        this.callListener = callListener;
    }

    @Override
    public String getClientName() {
        return clientName;
    }

    @Override
    public SingleCallMonitor<Response<?>> start(Call<?> call, int tryNum) {
        var request = call.request();
        var method = request.method();
        var url = request.url();

        var header = RequestIdProviders.X_REQUEST_ID;
        var requestId = call.request().header(header);

        return new SingleCallMonitor<>() {
            @Nullable
            private Response<?> response;
            @Nullable
            private String errorResponseText;

            private long getElapsed(CallContext context) {
                long started = context.start.get();
                long stopped = context.stop.get();
                if (started > 0) {
                    if (stopped > 0) {
                        return stopped - started;
                    } else {
                        return System.currentTimeMillis() - started;
                    }
                } else {
                    return -1;
                }
            }

            @Override
            public void success(@Nullable Response<?> response) {
                this.response = response;
                this.errorResponseText = null;

                var context = getContext(request);
                long elapsed = getElapsed(context);

                int retCode;
                if (response == null) {
                    retCode = NO_RESPONSE;
                    log.warn("[{}], Received {} [{}], [{}={}] took {} msec, NO RESPONSE",
                            clientName, method, url, header, requestId, elapsed);
                } else {
                    // It's generally unsafe to log all response headers and body
                    // Make sure to log only if we have to
                    retCode = response.code();
                    if (retCode >= 200 && retCode < 300) {
                        var loggingConfig = context.getLoggingConfig();

                        log.info("[{}], Received {} [{}], [{}={}] took {} msec, with code = {}{}{}",
                                clientName, method, url, header, requestId, elapsed, retCode,
                                loggingConfig.isLogResponseHeaders() ? "\nheaders = [" + getHeaders(context) + "]" : "",
                                loggingConfig.isLogResponseBody() ? "\nbody = [" + getBody(context) + "]" : "");
                    } else {
                        log.warn("[{}], Received {} [{}], [{}={}] took {} msec, with error code = {}, error body = {}",
                                clientName, method, url, header, requestId, elapsed, retCode, getCachedErrorResponse());
                    }
                }
                callListener.onCall(elapsed, retCode);
            }

            @Override
            public void failure(Exception t) {
                var context = getContext(request);
                long elapsed = getElapsed(context);
                var loggingConfig = context.getLoggingConfig();
                log.error("[{}], Received {} [{}], [{}={}] took {} msec, failed with error: {}{}{}",
                        clientName, method, url, header, requestId, elapsed, t.getMessage(),
                        loggingConfig.isLogResponseHeaders() ? "\nheaders = [" + getHeaders(context) + "]" : "",
                        loggingConfig.isLogResponseBody() ? "\nbody = [" + getBody(context) + "]" : "");
                callListener.onCall(elapsed, ERROR_REQUEST);
            }

            @Nullable
            @Override
            public String getCachedErrorResponse() {
                if (errorResponseText == null && response != null) {
                    errorResponseText = getErrorBody(response);
                }
                return errorResponseText;
            }
        };
    }

    //

    public static RetrofitLoggingCalls create(String clientName) {
        return create(clientName, CallListener.empty());
    }

    public static RetrofitLoggingCalls create(String clientName, CallListener callListener) {
        return new RetrofitLoggingCalls(clientName, callListener);
    }

    //

    static void configureContext(String clientName, Request request, Request.Builder builder) {
        var loggingConfig = Objects.requireNonNullElse(request.tag(LoggingConfig.class), LoggingConfig.standard());
        if (loggingConfig.isLogRequestHeaders()) {
            builder.addHeader(LOG_REQUEST_HEADERS, clientName);
        }
        if (loggingConfig.isLogRequestBody()) {
            builder.addHeader(LOG_REQUEST_BODY, clientName);
        }

        builder.tag(CallContext.class, new CallContext(clientName, loggingConfig));
    }

    static RequestFilter getRequestFilter() {
        return new RequestFilter() {
            @Override
            public <T> FilterContext<T> filter(FilterContext<T> ctx) {
                var request = ctx.getRequest();
                var headers = request.getHeaders();

                boolean logHeaders = false;
                boolean logBody = false;
                var clientName = headers.get(LOG_REQUEST_HEADERS);
                if (clientName != null) { // We should not use it anyway
                    headers.remove(LOG_REQUEST_HEADERS);
                    logHeaders = true;
                }

                String body = null;
                clientName = headers.get(LOG_REQUEST_BODY);
                if (clientName != null) {
                    headers.remove(LOG_REQUEST_BODY);
                    body = getRequestBodyAsString(request);
                    if (body != null) {
                        logBody = true;
                    }
                }

                if (logHeaders || logBody) {
                    var method = request.getMethod();
                    var url = request.getUrl();
                    var header = RequestIdProviders.X_REQUEST_ID;
                    var requestId = headers.get(header);
                    log.info("[{}],  Sending {} [{}], [{}={}]{}{}",
                            clientName, method, url, header, requestId,
                            logHeaders ? ", headers = [" + headers + "]" : "",
                            logBody ? ", body = [" + body + "]" : "");
                }

                return ctx;
            }
        };
    }

    static Consumer<AsyncHttpClientCall.AsyncHttpClientCallBuilder> getAsyncCallCustomizer() {
        return builder -> {
            builder.onRequestStart(request -> {
                getContext(request).start.set(System.currentTimeMillis());
            });
            builder.onRequestSuccess(response -> {
                var context = getContext(response.request());
                context.stop.set(System.currentTimeMillis());

                var loggingConfig = context.loggingConfig;
                if (context.loggingConfig.isLogResponseBody()) {
                    var body = response.body();
                    if (body != null) {
                        var contentLength = body.contentLength();
                        if (loggingConfig.isLimitResponseBodySize() && contentLength > FILTER_BODY_SIZE_LENGTH) {
                            context.responseBody.set("body of " + contentLength + " omitted");
                        } else {
                            var buffer = body.source().getBuffer().copy(); // TODO: avoid copy the buffer?
                            context.responseBody.set(buffer.readString(StandardCharsets.UTF_8));
                        }
                    }
                }
                if (context.loggingConfig.isLogResponseHeaders()) {
                    var headers = response.headers();
                    context.responseHeaders.set(headers.toString());
                }
            });
        };
    }

    //

    @Nullable
    private static String getRequestBodyAsString(org.asynchttpclient.Request request) {
        if (!StringUtils.isEmpty(request.getStringData())) {
            return request.getStringData();
        }
        if (request.getByteData() != null) {
            return new String(request.getByteData(), request.getCharset());
        }
        return null;
    }

    private static String getBody(CallContext context) {
        var body = context.responseBody.get();
        return Objects.requireNonNull(body, "<no body>");
    }

    private static String getHeaders(CallContext context) {
        var headers = context.responseHeaders.get();
        return Objects.requireNonNullElse(headers, "<no headers>");
    }

    private static String getErrorBody(Response<?> response) {
        var body = response.errorBody();
        try {
            return body != null ? body.string() : "<null>";
        } catch (IOException e) {
            log.error("Unable to collect response error body as string", e);
            return "<error>";
        }
    }

    private static CallContext getContext(Request request) {
        var requestContext = request.tag(CallContext.class);
        Preconditions.checkState(requestContext != null,
                "Internal error. Request context is not configured");
        return requestContext;
    }

    @Value
    static class CallContext {
        String clientName;
        LoggingConfig loggingConfig;
        AtomicLong start = new AtomicLong();
        AtomicLong stop = new AtomicLong();
        AtomicReference<String> responseBody = new AtomicReference<>();
        AtomicReference<String> responseHeaders = new AtomicReference<>();
    }
}
