package ru.yandex.ci.client.sandbox.api;

import java.util.List;

import lombok.ToString;
import lombok.Value;

@Value
public class DelegationResultList {
    List<DelegationResult> items;

    @Value
    public static class DelegationResult {
        boolean delegated;
        String message;

        @ToString.Exclude
        String secret;
    }
}
