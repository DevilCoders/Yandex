package ru.yandex.monlib.metrics.encode.json;

import java.nio.charset.StandardCharsets;


/**
 * @author Sergey Polovko
 */
public enum JsonConstant {
    TS("ts"),
    VALUE("value"),
    LABELS("labels"),
    SENSORS("sensors"),
    MEM_ONLY("memOnly"),
    MODE("mode"),
    TYPE("type"),
    KIND("kind"),
    TIMESERIES("timeseries"),
    DGAUGE("DGAUGE"),
    IGAUGE("IGAUGE"),
    COUNTER("COUNTER"),
    RATE("RATE"),
    HIST("HIST"),
    HIST_RATE("HIST_RATE"),
    BOUNDS("bounds"),
    BUCKETS("buckets"),
    INF("inf"),
    HIST_FIELD("hist")
    ;

    private final byte[] bytes;

    JsonConstant(String value) {
        this.bytes = value.getBytes(StandardCharsets.UTF_8);
    }

    public byte[] getBytes() {
        return bytes;
    }
}
