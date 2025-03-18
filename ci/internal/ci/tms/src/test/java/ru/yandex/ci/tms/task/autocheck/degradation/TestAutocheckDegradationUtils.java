package ru.yandex.ci.tms.task.autocheck.degradation;

import java.time.LocalDateTime;
import java.time.ZoneOffset;

import io.netty.handler.codec.http.HttpHeaderNames;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.Header;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.MediaType;
import org.springframework.http.HttpMethod;

import ru.yandex.ci.client.sandbox.SandboxDateTimeFormatter;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.ci.util.UserUtils;

public class TestAutocheckDegradationUtils {
    public static final String ROBOT_LOGIN = UserUtils.loginForInternalCiProcesses();

    private TestAutocheckDegradationUtils() {
    }

    public static void setUpPostcommitsSemaphores(
            ClientAndServer sandboxServer,
            int capacityAutocheck, int valueAutocheck,
            int capacityAutocheckForBranch, int valueAutocheckForBranch,
            int capacityPessimizedPrecommit, int valuePessimizedPrecommit,
            String authorAutocheck,
            String authorAutocheckForBranch
    ) {

        sandboxServer.clear(HttpRequest.request("/v1.0/semaphore/\\d+").withMethod(HttpMethod.GET.name()));
        sandboxServer.clear(HttpRequest.request("/v1.0/semaphore/\\d+/audit"));

        sandboxServer.when(HttpRequest.request("/v1.0/semaphore/12622386").withMethod(HttpMethod.GET.name()))
                .respond(getSemaphoreResponse(capacityAutocheck, valueAutocheck, LocalDateTime.of(2019, 1, 1, 1, 1)));
        sandboxServer.when(HttpRequest.request("/v1.0/semaphore/12622386/audit").withMethod(HttpMethod.GET.name()))
                .respond(getSemaphoreAuditResponse(LocalDateTime.of(2019, 1, 1, 1, 1), authorAutocheck));

        sandboxServer.when(
                HttpRequest.request("/v1.0/semaphore/16101546").withMethod(HttpMethod.GET.name())
        ).respond(getSemaphoreResponse(capacityAutocheckForBranch, valueAutocheckForBranch, LocalDateTime.of(2019, 1,
                1, 1, 1)));
        sandboxServer.when(
                HttpRequest.request("/v1.0/semaphore/16101546/audit").withMethod(HttpMethod.GET.name())
        ).respond(getSemaphoreAuditResponse(
                LocalDateTime.of(2019, 1, 1, 1, 1),
                authorAutocheckForBranch
        ));

        sandboxServer.when(
                HttpRequest.request("/v1.0/semaphore/15421035").withMethod(HttpMethod.GET.name())
        ).respond(getSemaphoreResponse(capacityPessimizedPrecommit, valuePessimizedPrecommit, LocalDateTime.of(2019, 1,
                1, 1, 1)));
        sandboxServer.when(
                HttpRequest.request("/v1.0/semaphore/15421035/audit").withMethod(HttpMethod.GET.name())
        ).respond(getSemaphoreAuditResponse(
                LocalDateTime.of(2019, 1, 1, 1, 1),
                authorAutocheckForBranch
        ));
    }

    public static HttpResponse getHttpResponseFromResource(String resourcePath) {
        return HttpResponse.response()
                .withStatusCode(HttpStatusCode.OK_200.code())
                .withHeader(Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                        MediaType.APPLICATION_JSON.toString()))
                .withBody(ResourceUtils.textResource(resourcePath));
    }

    private static HttpResponse getSemaphoreResponse(int capacity, int value, LocalDateTime updated) {
        return HttpResponse.response()
                .withStatusCode(HttpStatusCode.OK_200.code())
                .withHeader(Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                        MediaType.APPLICATION_JSON.toString()))
                .withBody("""
                        {
                            "capacity": %s,
                            "value": %s,
                            "time":{
                                "updated": "%s",
                                "created":"2019-03-19T17:12:23.643000Z"
                            }
                        }
                        """.formatted(capacity, value, formatDateTime(updated)));
    }

    private static HttpResponse getSemaphoreAuditResponse(LocalDateTime updated, String author) {
        return HttpResponse.response()
                .withStatusCode(HttpStatusCode.OK_200.code())
                .withHeader(Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                        MediaType.APPLICATION_JSON.toString()))
                .withBody("""
                        [
                            {
                                "source":"2a02:6b8:0:408:4d67:7a37:85ae:b6ca",
                                "description":"capacity changed",
                                "author":"%s",
                                "target":"sandbox-server08",
                                "time":"%s"
                            }
                        ]
                        """.formatted(author, formatDateTime(updated)));
    }

    private static String formatDateTime(LocalDateTime updated) {
        return SandboxDateTimeFormatter.format(updated.atOffset(ZoneOffset.UTC));
    }
}
