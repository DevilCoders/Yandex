package ru.yandex.ci.client.juggler;

import java.util.List;

import ru.yandex.ci.client.juggler.model.RawEvent;
import ru.yandex.ci.client.juggler.model.RawEventsResponse;

public interface JugglerPushClient {
    RawEventsResponse push(List<RawEvent> events);
}
