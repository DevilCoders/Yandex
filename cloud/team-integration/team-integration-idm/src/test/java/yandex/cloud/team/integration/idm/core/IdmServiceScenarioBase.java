package yandex.cloud.team.integration.idm.core;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.inject.Inject;

import lombok.extern.log4j.Log4j2;
import org.assertj.core.api.Assertions;
import org.assertj.core.api.ThrowableAssert;
import org.jetbrains.annotations.NotNull;
import org.junit.Before;
import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.fake.iam.FakeUser;
import yandex.cloud.scenario.AbstractContextScenario;
import yandex.cloud.team.integration.abc.ResolveServiceFacade;
import yandex.cloud.team.integration.idm.TestResources;
import yandex.cloud.ti.rm.client.MockResourceManagerClient;
import yandex.cloud.util.Json;

@Log4j2
public abstract class IdmServiceScenarioBase extends AbstractContextScenario<IdmServiceTestContext> {

    private static final String APPLICATION_X_WWW_FORM_URLENCODED = "application/x-www-form-urlencoded; charset=UTF-8";

    @Inject
    private static ResolveServiceFacade resolveServiceFacade;

    @Inject
    private static MockResourceManagerClient mockResourceManagerClient;

    @Inject
    protected static HttpServer httpServer;

    protected <T> T form(Class<T> entityType, String path, Map<String, Object> fields) {
        return send(entityType, path, APPLICATION_X_WWW_FORM_URLENCODED, ofFormFields(fields));
    }

    protected <T> T get(Class<T> entityType, String path) {
        return send(entityType, path, null, null);
    }

    private <T> T send(Class<T> entityType, String path, String contentType, HttpRequest.BodyPublisher body) {
        var uri = URI.create("http://localhost:" + httpServer.getLocalPort() + path);
        HttpClient httpClient = HttpClient.newBuilder().build();

        var request = HttpRequest.newBuilder().uri(uri);
        if (contentType != null) {
            request = request.header("Content-Type", contentType);
        }
        if (body != null) {
            request = request.POST(body);
        }
        HttpResponse<String> response;
        try {
            response = httpClient.send(request.build(), HttpResponse.BodyHandlers.ofString());
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
        if (response.statusCode() != HttpURLConnection.HTTP_OK) {
            throw new HttpResponseException(response);
        }

        return response.body().isEmpty() ? null : Json.fromJson(entityType, response.body());
    }

    protected void assertBadRequest(ThrowableAssert.ThrowingCallable callable) {
        assertErrorResponse(HttpURLConnection.HTTP_BAD_REQUEST, callable);
    }

    protected void assertBadMethod(ThrowableAssert.ThrowingCallable callable) {
        assertErrorResponse(HttpURLConnection.HTTP_BAD_METHOD, callable);
    }

    protected void assertPreconditionFailed(ThrowableAssert.ThrowingCallable callable) {
        assertErrorResponse(HttpURLConnection.HTTP_PRECON_FAILED, callable);
    }

    protected void assertErrorResponse(int errorCode, ThrowableAssert.ThrowingCallable callable) {
        Assertions.assertThatThrownBy(callable)
                .isInstanceOf(HttpResponseException.class)
                .hasMessageContaining("HTTP " + errorCode);
    }

    protected @NotNull List<MockResourceManagerClient.AccessBindingsRequest> getAccessBindingsRequests() {
        return mockResourceManagerClient.getAccessBindingsRequests();
    }

    protected static String getCloudIdForAbcService() {
        return resolveServiceFacade.resolveAbcServiceCloudByAbcServiceId(TestResources.ABC_SERVICE_ID).cloudId();
    }

    protected static Map<String, Object> getFields() {
        Map<String, Object> fields = new HashMap<>();
        fields.put("login", FakeUser.USER1_TOKEN.getUserId());
        fields.put("role", "{\"cloud\": \"s3\", \"storage\": \"admin\"}");
        fields.put("path", "/cloud/s3/storage/admin/");
        fields.put("fields", TestResources.EXTRA_FIELDS);
        return fields;
    }

    static HttpRequest.BodyPublisher ofFormFields(Map<String, Object> data) {
        var builder = new StringBuilder();
        for (var entry : data.entrySet()) {
            if (builder.length() > 0) {
                builder.append("&");
            }
            builder.append(URLEncoder.encode(entry.getKey(), StandardCharsets.UTF_8));
            builder.append("=");
            builder.append(URLEncoder.encode(entry.getValue().toString(), StandardCharsets.UTF_8));
        }
        return HttpRequest.BodyPublishers.ofString(builder.toString());
    }

    @Before
    public void clearAccessBindingsRequests() {
        mockResourceManagerClient.clearAccessBindingsRequests();
    }

}
