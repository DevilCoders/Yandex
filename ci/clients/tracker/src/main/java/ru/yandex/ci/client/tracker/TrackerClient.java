package ru.yandex.ci.client.tracker;

import java.net.URI;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.startrek.client.Session;
import ru.yandex.startrek.client.StartrekClient;
import ru.yandex.startrek.client.StartrekClientBuilder;
import ru.yandex.startrek.client.auth.OAuthAuthenticationInterceptor;
import ru.yandex.startrek.client.model.Field;

public class TrackerClient {
    public static final String FIELD_STATUS_START_TIME = "statusStartTime";

    private final StartrekClient client;

    private TrackerClient(String trackerUrl) {
        this.client = StartrekClientBuilder.newBuilder()
                .uri(trackerUrl)
                .maxConnections(10)
                .connectionTimeout(5, TimeUnit.SECONDS)
                .socketTimeout(30, TimeUnit.SECONDS)
                .customFields(Cf.map(FIELD_STATUS_START_TIME, Field.Schema.scalar(Field.Schema.Type.DATETIME, false)))
                .build();
    }

    public URI getEndpoint() {
        return client.getEndpoint();
    }

    public static TrackerClient create(@Nonnull String url) {
        return new TrackerClient(url);
    }


    public Session getSession(@Nonnull String token) {
        return client.getSession(new OAuthAuthenticationInterceptor(token));
    }

}
