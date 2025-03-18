package ru.yandex.ci.demo.woodflow;

import java.nio.file.Files;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.stream.Collectors;

import WoodflowCi.Woodflow.Board;
import WoodflowCi.furniture_factory.FurnitureFactory.Input;
import WoodflowCi.furniture_factory.FurnitureFactory.Output;
import com.google.common.collect.Lists;

import ru.yandex.tasklet.Result;
import ru.yandex.tasklet.TaskletContext;
import ru.yandex.tasklet.TaskletRuntime;

public class FurnitureFactory {

    private static final int WARDROBE_BOARDS_COUNT = 3;

    private final boolean sleep;

    public FurnitureFactory(boolean sleep) {
        this.sleep = sleep;
    }

    public Result<Output> execute(Input input, TaskletContext context) throws InterruptedException {
        var output = Output.newBuilder();

        var boards = input.getBoardsList().stream()
                .collect(Collectors.groupingBy(
                        board -> board.getSource().getName(),
                        LinkedHashMap::new,
                        Collectors.toList()
                ));

        for (var e : boards.entrySet()) {
            var boardType = e.getKey();
            var sameBoards = List.copyOf(e.getValue());

            for (var chunk : Lists.partition(sameBoards, WARDROBE_BOARDS_COUNT)) {
                if (sleep) {
                    Thread.sleep(1000);
                }
                if (chunk.size() < WARDROBE_BOARDS_COUNT) {
                    output.addAllRemain(chunk);
                    break;
                }

                output.addFurnituresBuilder()
                        .setType("Шкаф")
                        .setDescription(describeWardrobe(boardType, chunk));
            }
        }

        var resource = context.getSandboxResourcesContext().createResource(
                "example-output.pb",
                meta -> meta.setType("BUILD_LOGS")
                        .setDescription("Some example message")
                        .putAttributes("ttl", "1"),
                path -> Files.writeString(path, output.toString())
        );
        output.addSandboxResources(resource);

        return Result.of(output.build());
    }

    private static String describeWardrobe(String boardType, List<Board> boards) {
        var producers = boards.stream()
                .map(Board::getProducer)
                .sorted()
                .distinct()
                .collect(Collectors.joining(" и "));

        return "Шкаф из %s досок, полученных из материала '%s', произведенного %s".formatted(
                boards.size(), boardType, producers);
    }

    public static void main(String[] args) {
        var impl = new FurnitureFactory(true);
        TaskletRuntime.execute(
                Input.class,
                Output.class,
                args,
                impl::execute
        );
    }
}
