package ru.yandex.ci.client.trendbox;

import java.util.List;

import lombok.AccessLevel;
import lombok.Getter;
import lombok.Value;

@Value
public class TrendboxListResponse<T> {
    int statusCode;
    String message;

    @Getter(AccessLevel.NONE)
    Payload<T> payload;

    public List<T> getItems() {
        return payload.getItems();
    }

    public int getTotalCount() {
        return payload.getCount();
    }

    @Value
    private static class Payload<T> {
        List<T> items;
        int count;
    }
}
