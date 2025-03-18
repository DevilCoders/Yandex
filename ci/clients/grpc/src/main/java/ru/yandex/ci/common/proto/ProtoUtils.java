package ru.yandex.ci.common.proto;

import com.google.common.base.Preconditions;
import com.google.protobuf.Descriptors;
import com.google.protobuf.GeneratedMessageV3;

public class ProtoUtils {

    private ProtoUtils() {
        //
    }

    public static Descriptors.Descriptor getMessageDescriptor(Class<?> type) {
        checkClass(type);
        try {
            return (Descriptors.Descriptor) type.getMethod("getDescriptor").invoke(null);
        } catch (Exception ex) {
            throw new RuntimeException(ex);
        }
    }

    public static GeneratedMessageV3.Builder<?> getMessageBuilder(Class<?> type) {
        checkClass(type);
        try {
            return (GeneratedMessageV3.Builder<?>) type.getMethod("newBuilder").invoke(null);
        } catch (Exception ex) {
            throw new RuntimeException(ex);
        }
    }

    private static void checkClass(Class<?> type) {
        Preconditions.checkState(
                GeneratedMessageV3.class.isAssignableFrom(type),
                "Class %s expected to be generated protobuf message",
                type
        );
    }
}
