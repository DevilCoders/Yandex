package ru.yandex.ci.flow.engine.source_code.model;

import java.util.Map;
import java.util.UUID;

public interface ResourceConsumer {
    Map<UUID, ConsumedResource> getConsumedResources();
}
