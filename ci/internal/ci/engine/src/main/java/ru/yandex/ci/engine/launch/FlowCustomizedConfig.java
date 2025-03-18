package ru.yandex.ci.engine.launch;

import java.util.function.Consumer;

import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.launch.FlowReference;

@Builder
@Value
public class FlowCustomizedConfig {
    public static final FlowCustomizedConfig EMPTY = FlowCustomizedConfig.builder().build();

    @Nullable
    FlowReference flowReference;

    @Nullable
    JsonObject flowVars;

    @Nullable
    Consumer<String> flowIdListener;

}
