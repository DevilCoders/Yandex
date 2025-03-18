package ru.yandex.ci.engine.event;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.common.hash.Hashing;
import lombok.Value;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.engine.event.proto.CiEvent;
import ru.yandex.ci.util.jackson.JacksonProtobufDeserializer;
import ru.yandex.ci.util.jackson.JacksonProtobufSerializer;

@Value
public class CiEventPayload implements BaseTemporalWorkflow.Id {

    @JsonSerialize(using = JacksonProtobufSerializer.class)
    @JsonDeserialize(using = JacksonProtobufDeserializer.class)
    CiEvent payload;

    public static CiEventPayload ofEvent(CiEvent event) {
        return new CiEventPayload(event);
    }

    @Override
    public String getTemporalWorkflowId() {
        var hash = Hashing.sipHash24()
                .hashBytes(getPayload().toByteArray())
                .toString();
        return hash;
    }
}
