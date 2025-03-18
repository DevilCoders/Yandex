package ru.yandex.ci.tms.client;

import java.time.Instant;
import java.util.List;

import io.netty.handler.codec.http.HttpHeaderNames;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.Header;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.HttpStatusCode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.client.juggler.JugglerClient;
import ru.yandex.ci.client.juggler.model.ChecksStatusRequest;
import ru.yandex.ci.client.juggler.model.FilterRequest;
import ru.yandex.ci.client.juggler.model.MuteCreateRequest;
import ru.yandex.ci.client.juggler.model.MuteFilterRequest;
import ru.yandex.ci.client.juggler.model.MuteInfoResponse;
import ru.yandex.ci.client.juggler.model.MuteRemoveRequest;
import ru.yandex.ci.client.juggler.model.MutesRequest;
import ru.yandex.ci.client.juggler.model.Status;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.JugglerClientTestConfig;
import ru.yandex.ci.util.ResourceUtils;

import static org.mockserver.model.JsonBody.json;

@ContextConfiguration(classes = {
        JugglerClientTestConfig.class
})
public class JugglerTest extends CommonTestBase {
    @Autowired
    public ClientAndServer jugglerServer;

    @Autowired
    public JugglerClient jugglerClient;

    @BeforeEach
    void setUp() {
        jugglerServer.clear(HttpRequest.request("/v2/checks/get_checks_state"));
        jugglerServer.clear(HttpRequest.request("/v2/mutes/get_mutes"));
        jugglerServer.clear(HttpRequest.request("/v2/mutes/set_mutes"));
        jugglerServer.clear(HttpRequest.request("/v2/mutes/remove_mutes"));
    }

    @Test
    void getChecksStatus() {
        jugglerServer.when(HttpRequest.request("/v2/checks/get_checks_state")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_get_checks_state_by_tags.json"))))
                .respond(HttpResponse.response()
                        .withStatusCode(HttpStatusCode.OK_200.code())
                        .withHeader(Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                                MediaType.APPLICATION_JSON.toString()))
                        .withBody(ResourceUtils.textResource(
                                "server_responses/juggler_get_checks_state_by_tags_ok.json")));

        var request = new ChecksStatusRequest(
                List.of(new FilterRequest("", "devtools.autocheck", "",
                        List.of("autocheck_degradation_full"))),
                100);

        var statuses = jugglerClient.getChecksStatus(request);

        Assertions.assertEquals(1, statuses.size());
        Assertions.assertEquals(Status.OK, statuses.get(0));
    }

    @Test
    void getMutes() {
        jugglerServer.when(HttpRequest.request("/v2/mutes/get_mutes")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_get_mutes.json"))))
                .respond(HttpResponse.response()
                        .withStatusCode(HttpStatusCode.OK_200.code())
                        .withHeader(Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                                MediaType.APPLICATION_JSON.toString()))
                        .withBody(ResourceUtils.textResource(
                                "server_responses/juggler_get_mutes_ok.json")));

        var request = new MutesRequest(
                List.of(new MuteFilterRequest(
                        "some-host",
                        "devtools.autocheck",
                        "some-service",
                        List.of("autocheck_degradation_full", "t2"),
                        "username")),
                1,
                10,
                true);

        var statuses = jugglerClient.getMutes(request);

        Assertions.assertEquals(1, statuses.size());
        Assertions.assertEquals(
                new MuteInfoResponse("id-1", "user-1", Instant.ofEpochMilli(1000), Instant.ofEpochMilli(2000)),
                statuses.get(0));
    }

    @Test
    void createMute() {
        jugglerServer.when(HttpRequest.request("/v2/mutes/set_mutes")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_set_mutes.json"))))
                .respond(HttpResponse.response()
                        .withStatusCode(HttpStatusCode.OK_200.code()));

        var request = new MuteCreateRequest(
                "some-desc",
                Instant.ofEpochMilli(1000),
                Instant.ofEpochMilli(2000),
                List.of(new FilterRequest(
                        "some-host",
                        "devtools.autocheck",
                        "some-service",
                        List.of("autocheck_degradation_full", "t2"))));

        jugglerClient.createMute(request);
    }

    @Test
    void removeMutes() {
        jugglerServer.when(HttpRequest.request("/v2/mutes/remove_mutes")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_remove_mutes.json"))))
                .respond(HttpResponse.response()
                        .withStatusCode(HttpStatusCode.OK_200.code()));

        var request = new MuteRemoveRequest(
                List.of("id-1", "id-2"));

        jugglerClient.removeMutes(request);
    }
}
