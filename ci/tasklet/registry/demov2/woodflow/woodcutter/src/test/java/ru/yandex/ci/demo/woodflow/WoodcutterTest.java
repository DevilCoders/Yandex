package ru.yandex.ci.demo.woodflow;

import java.io.IOException;

import WoodflowCi.woodcutter.Woodcutter.Input;
import WoodflowCi.woodcutter.Woodcutter.Output;
import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import ru.yandex.tasklet.test.TaskletContextStub;

import static org.junit.jupiter.api.Assertions.assertEquals;

class WoodcutterTest {

    static {
        Misc.fixProtobufPrinter();
    }

    private static TaskletContextStub stub;

    @BeforeAll
    static void initAll() {
        stub = TaskletContextStub.stub(Input.class, Output.class);
    }

    @AfterAll
    static void closeAll() {
        stub.close();
    }

    @Test
    void simpleTest() throws IOException, InterruptedException {
        var input = Misc.fromProtoText("input.pb", Input.class);
        var output = Misc.fromProtoText("output.pb", Output.class);

        assertEquals(
                output,
                new Woodcutter(false).execute(input, stub.getContext()).getResult()
        );
    }

}
