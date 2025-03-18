package ru.yandex.ci.client.juggler;

import java.util.List;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.juggler.model.RawEvent;
import ru.yandex.ci.client.juggler.model.RawEvents;
import ru.yandex.ci.client.juggler.model.RawEventsResponse;

public class JugglerPushClientImpl implements JugglerPushClient {
    private final JugglerPushApi api;

    private JugglerPushClientImpl(HttpClientProperties httpClientProperties) {
        var objectMapper = RetrofitClient.Builder.defaultObjectMapper();
        objectMapper.registerModule(new JugglerModule());

        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .objectMapper(objectMapper)
                .build(JugglerPushApi.class);
    }

    public static JugglerPushClient create(HttpClientProperties httpClientProperties) {
        return new JugglerPushClientImpl(httpClientProperties);
    }

    @Override
    public RawEventsResponse push(List<RawEvent> events) {
        return api.push(RawEvents.of(events));
    }
}
