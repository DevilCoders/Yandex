package ru.yandex.ci.tms.test.woodflow;

import ci.tasklet.registry.demo.picklock.Schema.Input;
import ci.tasklet.registry.demo.picklock.Schema.Output;
import ci.tasklet.registry.demo.picklock.Schema.Picklock;
import com.google.protobuf.Descriptors;
import com.google.protobuf.TextFormat;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.tms.test.SandboxClientTaskletExecutorStub;

@Slf4j
public class PicklockStub implements SandboxClientTaskletExecutorStub.TaskletStub<Input, Output> {

    @Override
    public String implementationName() {
        return "Picklock";
    }

    @Override
    public Descriptors.Descriptor taskletMessage() {
        return Picklock.getDescriptor();
    }

    @Override
    public Class<Input> inputClass() {
        return Input.class;
    }

    @Override
    public Output execute(Input input) {
        log.info("Factory input: {}", TextFormat.shortDebugString(input));

        var builder = Output.newBuilder();
        for (var key : input.getYavFilter().getKeysList()) {
            var value = builder.addValuesBuilder()
                    .setKey(key)
                    .setValue("any");
            log.info("Produce: {}", value);
        }

        var output = builder.build();
        log.info("Factory output: {}", TextFormat.shortDebugString(output));
        return output;
    }
}
