package ru.yandex.ci.client.arcanum;

import javax.annotation.Nonnull;

import lombok.Value;

@Value
public class RegisterCheckRequestDto {
    @Nonnull
    String checkId;
    boolean fastCircuit;
    boolean newPlate;
}
