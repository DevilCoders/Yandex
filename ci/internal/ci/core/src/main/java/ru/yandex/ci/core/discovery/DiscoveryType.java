package ru.yandex.ci.core.discovery;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum DiscoveryType {
    DIR,
    GRAPH,
    STORAGE,
    PCI_DSS
}
