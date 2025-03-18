package ru.yandex.ci.common.proto;

import java.time.Instant;

import javax.annotation.Nullable;

import com.google.protobuf.Timestamp;

public class ProtoConverter {
    private ProtoConverter() {
    }

    public static Timestamp convert(@Nullable Instant instant) {
        if (instant == null) {
            return Timestamp.newBuilder().build();
        }

        return Timestamp.newBuilder()
                .setSeconds(instant.getEpochSecond())
                .setNanos(instant.getNano())
                .build();
    }

    public static Instant convert(Timestamp timestamp) {
        return Instant.ofEpochSecond(timestamp.getSeconds(), timestamp.getNanos());
    }
}
