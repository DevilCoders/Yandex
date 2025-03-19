package yandex.cloud.ti.yt.abc.client;

import java.time.Duration;

import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import okhttp3.mockwebserver.RecordedRequest;
import org.assertj.core.api.Assertions;
import org.jetbrains.annotations.NotNull;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import yandex.cloud.iam.client.tvm.TvmClientFactory;
import yandex.cloud.ti.http.client.HttpClientException;

public class TeamAbcClientTest {

    private MockWebServer mockWebServer;
    private TeamAbcClient teamAbcClient;


    @Before
    public void createTeamAbcClient() throws Exception {
        mockWebServer = new MockWebServer();
        mockWebServer.start();

        teamAbcClient = new TeamAbcClientImpl(
                "test",
                new TeamAbcClientProperties(
                        mockWebServer.getHostName(),
                        2,
                        mockWebServer.getPort(),
                        Duration.ofSeconds(1L),
                        Duration.ofSeconds(1L),
                        false,
                        0
                ),
                TvmClientFactory.createStubTvmClient()
        );
    }

    @After
    public void cleanup() throws Exception {
        mockWebServer.shutdown();
    }


    @Test
    public void getAbcServiceById() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();
        String abcServiceSlug = TestTeamAbcServices.templateAbcServiceSlug(abcServiceId);
        String abcServiceName = TestTeamAbcServices.templateAbcServiceName(abcServiceId);

        prepareResponse("""
                {
                  "next": null,
                  "previous": null,
                  "results": [
                    {
                      "id": %1$d,
                      "name": {
                        "ru": "%3$s-ru",
                        "en": "%3$s-en"
                      },
                      "slug": "%2$s"
                    }
                  ]
                }
                """
                .formatted(
                        abcServiceId,
                        abcServiceSlug,
                        abcServiceName
                )
        );

        TeamAbcService service = teamAbcClient.getAbcServiceById(abcServiceId);
        Assertions.assertThat(service.id()).isEqualTo(abcServiceId);
        Assertions.assertThat(service.slug()).isEqualTo(abcServiceSlug);
        Assertions.assertThat(service.name()).isEqualTo(abcServiceName + "-en");

        assertRequest("/api/v4/services/?id=%d&fields=id,slug,name".formatted(abcServiceId));
    }

    @Test
    public void getAbcServiceByIdWhenIdIsZero() {
        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceById(0)
                )
                .isInstanceOf(IllegalArgumentException.class);
    }

    @Test
    public void getAbcServiceByIdWhenResponseResultsAreEmpty() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();

        prepareResponse("""
                {
                  "next": null,
                  "previous": null,
                  "results": []
                }
                """
        );

        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceById(abcServiceId)
                )
                .isInstanceOf(AbcServiceNotFoundException.class);

        assertRequest("/api/v4/services/?id=%d&fields=id,slug,name".formatted(abcServiceId));
    }

    @Test
    public void getAbcServiceByIdWhenResponseCodeIs500() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();

        mockWebServer.enqueue(new MockResponse()
                .setResponseCode(500)
        );

        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceById(abcServiceId)
                )
                .isInstanceOf(HttpClientException.class);

        assertRequest("/api/v4/services/?id=%d&fields=id,slug,name".formatted(abcServiceId));
    }

    @Test
    public void getAbcServiceByIdWhenResponseBodyIsEmpty() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();

        prepareResponse("{}");

        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceById(abcServiceId)
                )
                .isInstanceOf(HttpClientException.class);

        assertRequest("/api/v4/services/?id=%d&fields=id,slug,name".formatted(abcServiceId));
    }


    @Test
    public void getAbcServiceBySlug() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();
        String abcServiceSlug = TestTeamAbcServices.templateAbcServiceSlug(abcServiceId);
        String abcServiceName = TestTeamAbcServices.templateAbcServiceName(abcServiceId);

        prepareResponse("""
                {
                  "next": null,
                  "previous": null,
                  "results": [
                    {
                      "id": %1$d,
                      "name": {
                        "ru": "%3$s-ru",
                        "en": "%3$s-en"
                      },
                      "slug": "%2$s"
                    }
                  ]
                }
                """
                .formatted(
                        abcServiceId,
                        abcServiceSlug,
                        abcServiceName
                )
        );

        TeamAbcService service = teamAbcClient.getAbcServiceBySlug(abcServiceSlug);
        Assertions.assertThat(service.id()).isEqualTo(abcServiceId);
        Assertions.assertThat(service.slug()).isEqualTo(abcServiceSlug);
        Assertions.assertThat(service.name()).isEqualTo(abcServiceName + "-en");

        assertRequest("/api/v4/services/?slug=%s&fields=id,slug,name".formatted(abcServiceSlug));
    }

    @Test
    public void getAbcServiceBySlugWhenSlugIsEmpty() {
        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceBySlug("")
                )
                .isInstanceOf(IllegalArgumentException.class);
    }

    @Test
    public void getAbcServiceBySlugWhenResponseResultsAreEmpty() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();
        String abcServiceSlug = TestTeamAbcServices.templateAbcServiceSlug(abcServiceId);

        prepareResponse("""
                {
                  "next": null,
                  "previous": null,
                  "results": []
                }
                """
        );

        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceBySlug(abcServiceSlug)
                )
                .isInstanceOf(AbcServiceNotFoundException.class);

        assertRequest("/api/v4/services/?slug=%s&fields=id,slug,name".formatted(abcServiceSlug));
    }

    @Test
    public void getAbcServiceBySlugWhenResponseCodeIs500() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();
        String abcServiceSlug = TestTeamAbcServices.templateAbcServiceSlug(abcServiceId);

        mockWebServer.enqueue(new MockResponse()
                .setResponseCode(500)
        );


        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceBySlug(abcServiceSlug)
                )
                .isInstanceOf(HttpClientException.class);

        assertRequest("/api/v4/services/?slug=%s&fields=id,slug,name".formatted(abcServiceSlug));
    }

    @Test
    public void getAbcServiceBySlugWhenResponseBodyIsEmpty() throws Exception {
        long abcServiceId = TestTeamAbcServices.randomAbcServiceId();
        String abcServiceSlug = TestTeamAbcServices.templateAbcServiceSlug(abcServiceId);

        prepareResponse("{}");

        Assertions
                .assertThatThrownBy(() ->
                        teamAbcClient.getAbcServiceBySlug(abcServiceSlug)
                )
                .isInstanceOf(HttpClientException.class);

        assertRequest("/api/v4/services/?slug=%s&fields=id,slug,name".formatted(abcServiceSlug));
    }


    private void prepareResponse(
            @NotNull String body
    ) {
        mockWebServer.enqueue(new MockResponse()
                .setResponseCode(200)
                .addHeader("Content-Type", "application/json")
                .setBody(body)
        );
    }

    private void assertRequest(
            @NotNull String expectedPath
    ) throws Exception {
        RecordedRequest request = mockWebServer.takeRequest();
        Assertions.assertThat(request.getMethod()).isEqualTo("GET");
        Assertions.assertThat(request.getPath()).isEqualTo(expectedPath);
        Assertions.assertThat(request.getHeader("X-Ya-Service-Ticket")).isNotEmpty();
    }

}
