package ru.yandex.ci.core.proto;

import java.lang.reflect.Field;

import com.google.protobuf.Descriptors;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;

import ru.yandex.ci.common.proto.ProtoUtils;
import ru.yandex.ci.core.resources.Resource;

public class ProtobufReflectionUtils {
    private ProtobufReflectionUtils() {
    }

    public static GeneratedMessageV3.Builder<?> getMessageBuilder(Field field) {
        return getMessageBuilder(field.getType());
    }

    public static Descriptors.Descriptor getMessageDescriptor(Class<?> type) {
        return ProtoUtils.getMessageDescriptor(type);
    }

    public static GeneratedMessageV3.Builder<?> getMessageBuilder(Class<?> type) {
        return ProtoUtils.getMessageBuilder(type);
    }

    public static <T extends Message.Builder> T merge(Resource resource, T builder)
            throws InvalidProtocolBufferException {
        return ProtobufSerialization.deserializeFromGson(resource.getData(), builder);
    }
}
