package ru.yandex.ci.core.config.a.model;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.With;

@SuppressWarnings("ReferenceEquality")
@Value
@AllArgsConstructor
@JsonIgnoreProperties(ignoreUnknown = true)
public class AYamlConfig {

    @With
    @Nonnull
    String service;

    @Nonnull
    String title;

    @With
    CiConfig ci; // Could be null

    @Nullable
    SoxConfig sox;

}

