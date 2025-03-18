package ru.yandex.ci.storage.core.message.utils;

import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

public class ForwardMessageUtils {
    private ForwardMessageUtils() {

    }

    public static String formatForwardMessage(ShardOut.ShardForwardingMessage message) {
        return "%s/%d/%s".formatted(
                CheckProtoMappers.toTaskId(message.getFullTaskId()),
                message.getPartition(),
                message.getMessageCase()
        );
    }
}
