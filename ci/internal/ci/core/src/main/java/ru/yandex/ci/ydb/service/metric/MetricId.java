package ru.yandex.ci.ydb.service.metric;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import com.google.common.base.Splitter;
import io.micrometer.core.instrument.Tag;
import lombok.Value;
import one.util.streamex.StreamEx;

@Value
public class MetricId {
    private static final String DELIMITER = ":";
    private static final String KEY_VALUE_DELIMITER = "=";

    private static final Splitter SPLITTER = Splitter.on(DELIMITER);
    private static final Splitter KEY_VALUE_SPLITTER = Splitter.on(KEY_VALUE_DELIMITER).limit(2);

    String name;
    List<Tag> tags;

    public static MetricId of(String name, List<Tag> tags) {
        return new MetricId(name, StreamEx.of(tags).sorted().toImmutableList());
    }

    public static MetricId of(String name, Tag... tags) {
        return new MetricId(name, StreamEx.of(tags).sorted().toImmutableList());
    }

    public String asString() {
        return StreamEx.of(tags)
                .map(t -> t.getKey() + KEY_VALUE_DELIMITER + t.getValue())
                .prepend(name)
                .joining(DELIMITER);
    }

    public static MetricId ofString(String id) {
        Iterator<String> splitIterator = SPLITTER.split(id).iterator();
        String name = splitIterator.next();

        List<Tag> tags = new ArrayList<>();
        while (splitIterator.hasNext()) {
            Iterator<String> keyValueSplitIterator = KEY_VALUE_SPLITTER.split(splitIterator.next()).iterator();
            tags.add(Tag.of(keyValueSplitIterator.next(), keyValueSplitIterator.next()));
        }
        Collections.sort(tags);
        return new MetricId(name, Collections.unmodifiableList(tags));
    }

}
