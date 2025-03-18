package ru.yandex.ci.client.abc;

import java.util.List;

import io.netty.handler.codec.http.HttpMethod;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.model.HttpStatusCode;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.util.ResourceUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
class AbcClientTest {

    private final MockServerClient server;

    private AbcClient abcClient;

    AbcClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        abcClient = AbcClient.create(HttpClientPropertiesStub.of(server));
    }

    @Test
    void getServicesAbc500() {
        var services = List.of("aaas", "adfox");

        server.when(
                request("/api/v4/services/")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("fields", "id,slug,name,description,path")
                        .withQueryStringParameter("page_size", "200")
                        .withQueryStringParameter("slug__in", String.join(",", services))
        ).respond(response().withStatusCode(HttpStatusCode.INTERNAL_SERVER_ERROR_500.code()));

        Assertions.assertThatThrownBy(() -> abcClient.getServices(services)).hasMessageContaining("500");
    }

    @Test
    void getServicesSimple() {

        var services = List.of("aaas", "adfox");

        server.when(
                request("/api/v4/services/")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("fields", "id,slug,name,description,path")
                        .withQueryStringParameter("page_size", "200")
                        .withQueryStringParameter("slug__in", String.join(",", services))
        ).respond(response().withBody(resource("get-services-simple.json")));

        assertThat(abcClient.getServices(services))
                .isEqualTo(List.of(
                        new AbcServiceInfo(
                                737,
                                "adfox",
                                new AbcServiceInfo.LocalizedName("ADFOX", "ADFOX"),
                                new AbcServiceInfo.LocalizedName(
                                        "ADFOX - это технологии для управления рекламой (adserving)" +
                                                " для сайтов, сетей, агентств. ",
                                        "ADFOX - adserving service"
                                ),
                                "/meta_search/ad/rsy/adfox/"
                        ),
                        new AbcServiceInfo(
                                4517,
                                "aaas",
                                new AbcServiceInfo.LocalizedName("Horizon", "Horizon"),
                                new AbcServiceInfo.LocalizedName("AppHost as a Service.", "AppHost as a Service."),
                                "/meta_search/vs_search/searchinfrastructure/apphost/aaas/"
                        )
                ));

    }

    @Test
    void getAllServicesSimple() {
        server.when(
                request("/api/v4/services/")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("fields", "id,slug,name,description,path")
                        .withQueryStringParameter("page_size", "200")
        ).respond(response().withBody(resource("get-services-simple.json")));

        assertThat(abcClient.getAllServices())
                .isEqualTo(List.of(
                        new AbcServiceInfo(
                                737,
                                "adfox",
                                new AbcServiceInfo.LocalizedName("ADFOX", "ADFOX"),
                                new AbcServiceInfo.LocalizedName(
                                        "ADFOX - это технологии для управления рекламой (adserving)" +
                                                " для сайтов, сетей, агентств. ",
                                        "ADFOX - adserving service"
                                ),
                                "/meta_search/ad/rsy/adfox/"
                        ),
                        new AbcServiceInfo(
                                4517,
                                "aaas",
                                new AbcServiceInfo.LocalizedName("Horizon", "Horizon"),
                                new AbcServiceInfo.LocalizedName("AppHost as a Service.", "AppHost as a Service."),
                                "/meta_search/vs_search/searchinfrastructure/apphost/aaas/"
                        )
                ));

    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }
}
