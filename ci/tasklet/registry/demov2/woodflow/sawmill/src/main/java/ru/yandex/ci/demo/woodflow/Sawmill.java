package ru.yandex.ci.demo.woodflow;

import java.util.stream.IntStream;

import WoodflowCi.sawmill.Sawmill.Input;
import WoodflowCi.sawmill.Sawmill.Output;

import ru.yandex.tasklet.Result;
import ru.yandex.tasklet.TaskletContext;
import ru.yandex.tasklet.TaskletRuntime;

public class Sawmill {

    private final boolean sleep;

    public Sawmill(boolean sleep) {
        this.sleep = sleep;
    }

    public Result<Output> execute(Input input, TaskletContext context) throws InterruptedException {
        var output = Output.newBuilder();

        String title;
        int boardsPerTimber;
        if (!input.getDocument().getTitle().isEmpty()) {
            title = input.getDocument().getTitle();
            boardsPerTimber = input.getDocument().getBoardsPerTimber();
        } else {
            title = "Обычная лесопилка";
            boardsPerTimber = 3;
        }

        for (var timber : input.getTimbersList()) {
            for (int i : IntStream.range(0, boardsPerTimber).toArray()) {
                if (sleep) {
                    Thread.sleep(1000);
                }
                output.addBoardsBuilder()
                        .setSeq(i + 1)
                        .setProducer(title)
                        .setSource(timber);
            }
        }

        return Result.of(output.build());
    }

    public static void main(String[] args) {
        var impl = new Sawmill(true);
        TaskletRuntime.execute(
                Input.class,
                Output.class,
                args,
                impl::execute
        );
    }
}
