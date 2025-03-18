package ru.yandex.ci.util.jackson;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat;

public class JacksonProtobufSerializer extends JsonSerializer<Message> {
    private final JsonFormat.Printer compactPrinter = JsonFormat.printer().preservingProtoFieldNames()
            .omittingInsignificantWhitespace();

    @Override
    public void serialize(Message value, JsonGenerator gen, SerializerProvider serializers) throws IOException {
        gen.writeRawValue(compactPrinter.print(value));
    }
}
