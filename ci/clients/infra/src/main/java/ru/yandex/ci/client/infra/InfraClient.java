package ru.yandex.ci.client.infra;

import java.time.Instant;
import java.util.List;

import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.POST;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.infra.event.EventDto;

public class InfraClient {

    private final InfraApi infraApi;

    private InfraClient(HttpClientProperties httpClientProperties) {
        this.infraApi = RetrofitClient.builder(httpClientProperties, getClass())
                .build(InfraApi.class);
    }

    public static InfraClient create(HttpClientProperties httpClientProperties) {
        return new InfraClient(httpClientProperties);
    }

    public List<EventDto> getEvents(int serviceId, Instant from, Instant to) {
        return infraApi.getEvents(serviceId, from.getEpochSecond(), to.getEpochSecond());
    }

    public EventDto createEvent(EventDto event) {
        return infraApi.createEvent(event);
    }

    // https://infra-api.yandex-team.ru/swagger/
    interface InfraApi {
        @GET("/v1/events")
        List<EventDto> getEvents(
                @Query("serviceId") int serviceId,
                @Query("from") long fromUnixTimestamp,
                @Query("to") long toUnixTimestamp
        );

        @POST("/v1/events")
        EventDto createEvent(@Body EventDto event);
    }
}
