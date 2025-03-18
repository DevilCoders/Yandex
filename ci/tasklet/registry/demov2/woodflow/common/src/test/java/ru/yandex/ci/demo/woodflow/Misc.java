package ru.yandex.ci.demo.woodflow;

import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

import com.google.common.io.Resources;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;

public class Misc {

    private Misc() {
    }

    static void fixProtobufPrinter() {
        try {
            // Can't imagine someone would like to see non-ASCII symbols in proto, right?
            var fieldLookup = MethodHandles.privateLookupIn(Field.class, MethodHandles.lookup());
            var fieldModifiers = fieldLookup.findVarHandle(Field.class, "modifiers", int.class);

            var defaultField = TextFormat.Printer.class.getDeclaredField("DEFAULT");
            defaultField.setAccessible(true);
            fieldModifiers.set(defaultField, defaultField.getModifiers() & ~Modifier.FINAL);

            defaultField.set(null, TextFormat.printer().escapingNonAscii(false));
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    static <T extends Message> T fromProtoText(String resource, Class<T> type) throws IOException {
        var protoText = readResource(resource);
        return TextFormat.parse(protoText, type);
    }

    static String readResource(String resource) throws IOException {
        return Resources.toString(Resources.getResource(resource), StandardCharsets.UTF_8);
    }

    static String readFile(String fileName) throws IOException {
        return Files.readString(Path.of(fileName));
    }
}
