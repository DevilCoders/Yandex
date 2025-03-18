package ru.yandex.ci.core.launch;

import java.util.List;

import lombok.Value;

import yandex.cloud.binding.schema.Column;

@Value
public class LaunchUserData {
    @Column
    List<String> tags;
    @Column
    boolean pinned;
}
