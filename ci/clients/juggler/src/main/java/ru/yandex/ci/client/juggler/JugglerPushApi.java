package ru.yandex.ci.client.juggler;

import retrofit2.http.Body;
import retrofit2.http.POST;

import ru.yandex.ci.client.juggler.model.RawEvents;
import ru.yandex.ci.client.juggler.model.RawEventsResponse;

public interface JugglerPushApi {
    @POST("/events")
    RawEventsResponse push(@Body RawEvents events);
}
