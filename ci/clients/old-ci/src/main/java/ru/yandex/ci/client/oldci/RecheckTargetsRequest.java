package ru.yandex.ci.client.oldci;

import java.util.List;

import lombok.Value;

@Value
public class RecheckTargetsRequest {
    String action = "recheck_targets";
    List<CheckRecheckingTargets> checks;
}
