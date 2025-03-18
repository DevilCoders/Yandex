package ru.yandex.ci.client.yav.model;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import lombok.Value;


@Value
public class YavSecretsResponse {
    @Nullable
    List<YavSecret> secrets;

    public List<YavSecret> getSecrets() {
        return Objects.requireNonNullElse(secrets, List.of());
    }
}
