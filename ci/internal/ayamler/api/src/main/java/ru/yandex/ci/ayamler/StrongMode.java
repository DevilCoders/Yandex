package ru.yandex.ci.ayamler;

import java.util.Set;

import lombok.Value;
import lombok.With;

@Value
public class StrongMode {
    @With
    boolean enabled;
    Set<String> abcScopes;
}
