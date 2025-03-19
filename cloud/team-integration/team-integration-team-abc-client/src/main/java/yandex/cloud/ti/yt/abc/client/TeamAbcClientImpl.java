package yandex.cloud.ti.yt.abc.client;

import java.net.URLEncoder;
import java.net.http.HttpRequest;
import java.nio.charset.StandardCharsets;
import java.util.stream.Stream;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.http.Headers;
import yandex.cloud.iam.client.tvm.TvmClient;
import yandex.cloud.ti.http.client.AbstractHttpClient;

class TeamAbcClientImpl extends AbstractHttpClient implements TeamAbcClient {

    private static final String API_SERVICES = "/api/v4/services/";

    private final TvmClient tvmClient;
    private final int tvmId;


    TeamAbcClientImpl(
            @NotNull String userAgent,
            @NotNull TeamAbcClientProperties endpoint,
            @NotNull TvmClient tvmClient
    ) {
        super(endpoint, API_SERVICES, userAgent, null, null);
        this.tvmClient = tvmClient;
        tvmId = endpoint.getTvmId();
    }


    @Override
    public @NotNull TeamAbcService getAbcServiceById(
            long id
    ) {
        if (id <= 0) {
            throw new IllegalArgumentException("id not set");
        }
        var result = findService("id=" + id);
        if (result == null) {
            throw AbcServiceNotFoundException.forAbcId(id);
        }
        return result;
    }

    @Override
    public @NotNull TeamAbcService getAbcServiceBySlug(
            @NotNull String slug
    ) {
        if (slug.isEmpty()) {
            throw new IllegalArgumentException("slug not set");
        }
        var result = findService("slug=" + URLEncoder.encode(slug, StandardCharsets.UTF_8));
        if (result == null) {
            throw AbcServiceNotFoundException.forAbcSlug(slug);
        }
        return result;
    }

    private @Nullable TeamAbcService findService(
            @NotNull String filter
    ) {
        var query = filter + "&fields=id,slug,name";
        var response = sendRequest(null, query, null, TeamAbcServiceListResponse.class);
        return Stream.of(response.results())
                .findFirst()
                .map(it -> new TeamAbcService(
                        it.id(),
                        it.slug(),
                        it.name().en()
                ))
                .orElse(null);
    }

    @Override
    protected HttpRequest.Builder prepareRequest(
            @NotNull HttpRequest.Builder builder
    ) {
        return builder
                .setHeader(Headers.X_YA_SERVICE_TICKET, tvmClient.getServiceTicket(tvmId));
    }

}
