package ru.yandex.ci.client.taskletv2;

import java.util.function.Function;

import javax.annotation.Nonnull;

import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import lombok.Value;

import ru.yandex.tasklet.TaskletAction;
import ru.yandex.tasklet.test.TaskletContextStub;

@Value
public class TaskletExecutor<Input extends Message, Output extends Message> {
    @Nonnull
    Function<ByteString, Input> inputParser;
    @Nonnull
    Function<Output, ByteString> outputParser;
    @Nonnull
    TaskletContextStub stub;
    @Nonnull
    TaskletAction<Input, Output> action;
}
