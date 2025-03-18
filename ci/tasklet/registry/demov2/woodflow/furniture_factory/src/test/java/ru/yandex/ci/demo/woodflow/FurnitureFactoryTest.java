package ru.yandex.ci.demo.woodflow;

import java.io.IOException;
import java.nio.file.Path;

import WoodflowCi.furniture_factory.FurnitureFactory.Input;
import WoodflowCi.furniture_factory.FurnitureFactory.Output;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import ru.yandex.sandbox.tasklet.sidecars.resource_manager.proto.ResourceManagerApi;
import ru.yandex.tasklet.test.TaskletContextStub;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class FurnitureFactoryTest {
    static {
        Misc.fixProtobufPrinter();
    }

    @TempDir
    static Path tempDir;

    private TaskletContextStub stub;
    private FurnitureFactory tasklet;

    @BeforeEach
    void beforeEach() {
        stub = TaskletContextStub.stub(Input.class, Output.class, tempDir);
        tasklet = new FurnitureFactory(false);
    }

    @AfterEach
    void afterEach() {
        stub.close();
    }

    @Test
    void simpleTest() throws IOException, InterruptedException {
        var input = Misc.fromProtoText("input.pb", Input.class);
        var output = Misc.fromProtoText("output.pb", Output.class);

        var actual = tasklet.execute(input, stub.getContext()).getResult();
        assertEquals(output, actual);

        assertEquals(actual.getSandboxResourcesCount(), 1);

        var resource = stub.getContext().getSandboxResourceManager()
                .downloadResource(
                        ResourceManagerApi.DownloadResourceRequest.newBuilder()
                                .setId(actual.getSandboxResources(0).getId())
                                .build());

        assertTrue(resource.getPath().endsWith("/example-output.pb"));

        var expectText = Misc.readResource("output-file.txt");
        var actualText = Misc.readFile(resource.getPath());

        assertEquals(expectText, actualText);
    }
}
