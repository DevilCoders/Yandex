package ru.yandex.ci.demo.woodflow;

import java.util.List;

import WoodflowCi.Woodflow.Tree;
import WoodflowCi.woodcutter.Woodcutter.Input;
import WoodflowCi.woodcutter.Woodcutter.Output;

import ru.yandex.tasklet.Result;
import ru.yandex.tasklet.TaskletContext;
import ru.yandex.tasklet.TaskletRuntime;

public class Woodcutter {

    private final boolean sleep;

    public Woodcutter(boolean sleep) {
        this.sleep = sleep;
    }

    public Result<Output> execute(Input input, TaskletContext context) throws InterruptedException {
        var output = Output.newBuilder();

        var trees = input.getTreesCount() > 0
                ? input.getTreesList()
                : defaultTrees();

        for (var tree : trees) {
            if (sleep) {
                Thread.sleep(1000);
            }

            output.addTimbersBuilder()
                    .setName("бревно из дерева " + tree.getType());
        }

        return Result.of(output.build());
    }

    private static List<Tree> defaultTrees() {
        return List.of(
                makeTree("Бамбук обыкновенный"),
                makeTree("Ясень высокий")
        );
    }

    private static Tree makeTree(String name) {
        return Tree.newBuilder()
                .setType(name)
                .build();
    }

    public static void main(String[] args) {
        var impl = new Woodcutter(true);
        TaskletRuntime.execute(
                Input.class,
                Output.class,
                args,
                impl::execute
        );
    }
}
