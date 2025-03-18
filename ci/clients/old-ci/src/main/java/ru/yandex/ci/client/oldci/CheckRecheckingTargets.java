package ru.yandex.ci.client.oldci;

import java.util.List;

import lombok.Value;

@Value
public class CheckRecheckingTargets {
    String checkId;
    List<RecheckingTarget> recheckingTargets;
}
