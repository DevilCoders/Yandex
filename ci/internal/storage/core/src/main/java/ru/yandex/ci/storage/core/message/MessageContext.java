package ru.yandex.ci.storage.core.message;

import lombok.Value;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

@Value
public class MessageContext {
    MessageData messageData;
    LbCommitCountdown countdown;
    TimeTraceService.Trace trace;
}
