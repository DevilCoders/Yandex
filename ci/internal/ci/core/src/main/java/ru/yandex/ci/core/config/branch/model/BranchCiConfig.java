package ru.yandex.ci.core.config.branch.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class BranchCiConfig {
    @JsonProperty("delegated-config")
    String delegatedConfig;
    BranchAutocheckConfig autocheck;
}
