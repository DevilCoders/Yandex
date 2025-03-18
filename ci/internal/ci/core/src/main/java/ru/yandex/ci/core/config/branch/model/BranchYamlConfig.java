package ru.yandex.ci.core.config.branch.model;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.Value;

@Value
@JsonIgnoreProperties(ignoreUnknown = true)
public class BranchYamlConfig {
    String service;
    @Nullable
    BranchCiConfig ci;
}
