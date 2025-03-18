package ru.yandex.ci.engine.discovery.filter;

import ru.yandex.ci.core.discovery.DiscoveryType;

class DiscoveryTypeNotSupportedException extends IllegalArgumentException {
    DiscoveryTypeNotSupportedException(DiscoveryType discoveryType) {
        super("Path filters for %s discovery is not supported".formatted(discoveryType));
    }
}
