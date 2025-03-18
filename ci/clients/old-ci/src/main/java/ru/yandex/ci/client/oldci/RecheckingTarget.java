package ru.yandex.ci.client.oldci;

import lombok.Value;

@Value
public class RecheckingTarget {
    String toolchain;
    String path;
    boolean onlyWithPatch;
    int partition;
}
