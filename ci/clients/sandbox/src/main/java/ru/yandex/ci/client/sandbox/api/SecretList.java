package ru.yandex.ci.client.sandbox.api;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import lombok.ToString;
import lombok.Value;

@Value
public class SecretList {
    List<Secret> secrets;

    public static SecretList withSecrets(String... secretIds) {
        List<Secret> secrets = Stream.of(secretIds).map(Secret::new).collect(Collectors.toList());
        return new SecretList(secrets);
    }

    @Value
    public static class Secret {
        @ToString.Exclude
        String id;
    }
}
