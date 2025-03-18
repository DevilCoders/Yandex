package ru.yandex.ci.tms.test.woodflow;

import java.util.List;
import java.util.stream.Collectors;

import WoodflowCi.Woodflow;
import WoodflowCi.woodcutter.Woodcutter.Input;
import WoodflowCi.woodcutter.Woodcutter.Output;
import WoodflowCi.woodcutter.WoodcutterOuterClass;
import com.google.protobuf.Descriptors;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.tms.test.SandboxClientTaskletExecutorStub;

//TODO: switch to TaskletV2 and ru.yandex.ci.demo.woodflow.Woodcutter
@Slf4j
public class WoodcutterStub implements SandboxClientTaskletExecutorStub.TaskletStub<Input, Output> {

    private final List<String> defaultItems;

    public WoodcutterStub() {
        this(List.of());
    }

    public WoodcutterStub(List<String> defaultItems) {
        this.defaultItems = defaultItems;
    }

    @Override
    public String implementationName() {
        return "WoodcutterPy";
    }

    @Override
    public Descriptors.Descriptor taskletMessage() {
        return WoodcutterOuterClass.Woodcutter.getDescriptor();
    }

    @Override
    public Class<Input> inputClass() {
        return Input.class;
    }

    @Override
    public Output execute(Input input) {
        List<String> treeNames;

        if (!defaultItems.isEmpty()) {
            log.info("Use exact {} types", defaultItems.size());
            treeNames = defaultItems;
        } else if (input.getTreesCount() > 0) {
            log.info("Received {} types", input.getTreesCount());
            treeNames = input.getTreesList().stream()
                    .map(Woodflow.Tree::getType)
                    .collect(Collectors.toList());
        } else {
            log.info("Default values");
            treeNames = List.of("Бамбук обыкновенный", "Ясень высокий");
        }

        List<Woodflow.Timber> timbers = treeNames.stream()
                .map(tree -> Woodflow.Timber.newBuilder().setName("бревно из дерева " + tree).build())
                .collect(Collectors.toList());

        log.info("Produced {} timbers", timbers.size());
        return Output.newBuilder()
                .addAllTimbers(timbers)
                .build();
    }
}
