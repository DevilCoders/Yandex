package ru.yandex.ci.common.grpc;

import java.io.IOException;
import java.util.function.Function;

import com.google.protobuf.Descriptors;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import org.assertj.core.api.Assertions;

import ru.yandex.ci.common.proto.ProtoUtils;
import ru.yandex.ci.util.ResourceUtils;

public class ProtobufTestUtils {

    private static final TextFormat.Printer PROTO_PRINTER = TextFormat.printer().escapingNonAscii(false);

    private ProtobufTestUtils() {
        //
    }

    public static <T extends Message> T parseProtoText(String resource, Class<T> target) {
        return parseProtoTextFromString(ResourceUtils.textResource(resource), target);
    }

    public static <T extends Message> T parseProtoTextFromString(String protoText, Class<T> target) {
        try {
            Assertions.registerFormatterForType(target, representProtobuf(target));
            return TextFormat.parse(protoText, target);
        } catch (TextFormat.ParseException e) {
            throw new RuntimeException(e);
        }
    }

    @SuppressWarnings("unchecked")
    public static <T extends Message> T parseProtoBinary(String resource, Class<T> target) {
        var builder = ProtoUtils.getMessageBuilder(target);
        try (var stream = ResourceUtils.stream(resource)) {
            builder.mergeFrom(stream);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return (T) builder.build();
    }

    //

    private static <T extends Message> String protoTextHeader(Class<T> target) {
        try {
            Descriptors.Descriptor descriptor = (Descriptors.Descriptor) target.getMethod("getDescriptor").invoke(null);
            var file = descriptor.getFile().getFullName();
            var message = descriptor.getName();
            return """
                    # proto-file: %s
                    # proto-message: %s

                    """.formatted(file, message);
        } catch (ReflectiveOperationException e) {
            throw new RuntimeException(e);
        }
    }

    private static <T extends Message> Function<T, String> representProtobuf(Class<T> target) {
        return message -> protoTextHeader(target) + PROTO_PRINTER.printToString(message);
    }

}
