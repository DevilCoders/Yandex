package ru.yandex.ci.tms.test.woodflow;

import WoodflowCi.Woodflow;
import WoodflowCi.sawmill.Sawmill.Input;
import WoodflowCi.sawmill.Sawmill.Output;
import WoodflowCi.sawmill.Sawmill.SawmillDocument;
import WoodflowCi.sawmill.SawmillOuterClass;
import com.google.gson.JsonParser;
import com.google.protobuf.Descriptors;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.tms.test.SandboxClientTaskletExecutorStub;
import ru.yandex.ci.util.gson.GsonPreciseDeserializer;

//TODO: switch to TaskletV2 and ru.yandex.ci.demo.woodflow.Sawmill
@Slf4j
public class SawmillStub implements SandboxClientTaskletExecutorStub.TaskletStub<Input, Output> {

    @Override
    public String implementationName() {
        return "SawmillPy";
    }

    public static void main(String[] args) {
        var output = Output.newBuilder()
                .addBoards(Woodflow.Board.newBuilder()
                        .setSeq(13)
                        .setSource(Woodflow.Timber.newBuilder()
                                .setName("береза")
                                .build())
                        .setProducer("лесопилка")
                        .build())
                .addBoards(Woodflow.Board.newBuilder()
                        .setSeq(13)
                        .setSource(Woodflow.Timber.newBuilder()
                                .setName("береза")
                                .build())
                        .setProducer("лесопилка")
                        .build())
                .build();
        var json = ProtobufSerialization.serializeToJsonString(output);
        var jsonObject = JsonParser.parseString(json).getAsJsonObject();
        var map = GsonPreciseDeserializer.toMap(jsonObject);
        System.out.println(map);
    }

    @Override
    public Descriptors.Descriptor taskletMessage() {
        return SawmillOuterClass.Sawmill.getDescriptor();
    }

    @Override
    public Class<Input> inputClass() {
        return Input.class;
    }

    @Override
    public Output execute(Input input) {
        log.info("Received {} timbers", input.getTimbersCount());

        SawmillDocument document = input.hasDocument() ? input.getDocument() : defaultDocument();
        Output.Builder builder = Output.newBuilder();
        for (Woodflow.Timber timber : input.getTimbersList()) {
            for (int i = 0; i < document.getBoardsPerTimber(); i++) {
                builder.addBoards(
                        Woodflow.Board.newBuilder()
                                .setSeq(i)
                                .setProducer(document.getTitle())
                                .setSource(timber)
                                .build()
                );
            }
        }

        log.info("Produced {} boards", builder.getBoardsCount());
        return builder.build();
    }

    private SawmillDocument defaultDocument() {
        return SawmillDocument.newBuilder()
                .setTitle("Обычная лесопилка")
                .setBoardsPerTimber(3)
                .build();
    }
}
