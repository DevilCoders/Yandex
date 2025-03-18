package ru.yandex.ci.storage.core.logbroker;

import java.util.List;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

@Slf4j
public class LogbrokerLogger {
    private LogbrokerLogger() {

    }

    public static void logSend(
            ProducerWriteResponse result, StorageWriterBase.Message<?> message, LogbrokerWriter producer
    ) {
        log.info(
                "Message written, seqNo: {}, offset: {}, already written: {}, sourceId: {}, message info: {}",
                result.getSeqNo(),
                result.getOffset(),
                result.isAlreadyWritten(),
                producer.getSourceId(),
                message.toLogString()
        );
    }

    public static <T> void logReceived(List<T> records, Function<T, Common.MessageMeta> getMeta) {
        log.info(
                "Messages received: {}",
                records.stream()
                        .map(getMeta)
                        .map(Common.MessageMeta::getId)
                        .collect(Collectors.joining(", "))
        );
    }
}
