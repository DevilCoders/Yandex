package ru.yandex.ci.tms.test.woodflow;

import java.util.List;
import java.util.Set;

import WoodflowCi.Woodflow;
import WoodflowCi.furniture_factory.FurnitureFactory.Input;
import WoodflowCi.furniture_factory.FurnitureFactory.Output;
import WoodflowCi.furniture_factory.FurnitureFactoryOuterClass;
import com.google.common.collect.Lists;
import com.google.protobuf.Descriptors;
import com.google.protobuf.TextFormat;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.tms.test.SandboxClientTaskletExecutorStub;

//TODO: switch to TaskletV2 and ru.yandex.ci.demo.woodflow.FurnitureFactory
@Slf4j
public class FurnitureFactoryStub implements SandboxClientTaskletExecutorStub.TaskletStub<Input, Output> {

    private static final int WARDROBE_BOARDS_COUNT = 3;

    @Override
    public String implementationName() {
        return "FurnitureFactoryPy";
    }

    @Override
    public Descriptors.Descriptor taskletMessage() {
        return FurnitureFactoryOuterClass.FurnitureFactory.getDescriptor();
    }

    @Override
    public Class<Input> inputClass() {
        return Input.class;
    }

    @Override
    public Output execute(Input input) {
        log.info(" Factory input: {}", TextFormat.shortDebugString(input));
        for (var board : input.getBoardsList()) {
            log.info("[{}] Producer: [{}], Timber: {}",
                    board.getSeq(),
                    board.getProducer(),
                    board.getSource().getName());
        }

        var bySource = StreamEx.of(input.getBoardsList())
                .groupingBy(b -> b.getSource().getName());

        Output.Builder builder = Output.newBuilder();

        for (var sameSource : bySource.entrySet()) {
            // Стабилизуем вывод
            var sources = StreamEx.of(sameSource.getValue())
                    .sortedBy(Woodflow.Board::getProducer)
                    .toList();
            for (List<Woodflow.Board> boards : Lists.partition(sources, WARDROBE_BOARDS_COUNT)) {
                if (boards.size() < WARDROBE_BOARDS_COUNT) {
                    builder.addAllRemain(boards);
                    break;
                }

                builder.addFurnitures(makeWardrobe(sameSource.getKey(), boards));
            }
        }
        return builder.build();
    }

    private Woodflow.Furniture makeWardrobe(String source, List<Woodflow.Board> boards) {
        Set<String> producers = StreamEx.of(boards)
                .map(Woodflow.Board::getProducer)
                .toSet();


        String description = String.format("Шкаф из %s досок, полученных из материала '%s', произведенного %s",
                boards.size(), source, producers);

        return Woodflow.Furniture.newBuilder()
                .setType("Шкаф")
                .setDescription(description)
                .build();
    }
}
