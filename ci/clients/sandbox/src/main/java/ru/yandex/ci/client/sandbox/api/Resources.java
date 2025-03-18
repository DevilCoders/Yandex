package ru.yandex.ci.client.sandbox.api;

import java.util.List;

import com.google.common.annotations.VisibleForTesting;
import lombok.AllArgsConstructor;
import lombok.Value;

@Value
@AllArgsConstructor
public class Resources {
    List<ResourceInfo> items;

    long total;
    int limit;
    int offset;

    @VisibleForTesting
    public Resources(List<ResourceInfo> items) {
        this(items, items.size(), 0, 0);
    }
}
