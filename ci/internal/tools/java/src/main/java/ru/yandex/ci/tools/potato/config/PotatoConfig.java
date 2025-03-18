package ru.yandex.ci.tools.potato.config;

import java.util.List;

import lombok.Value;

@Value
public class PotatoConfig {
    List<Handler> handlers;
}

