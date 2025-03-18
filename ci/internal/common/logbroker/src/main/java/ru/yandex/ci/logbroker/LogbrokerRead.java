package ru.yandex.ci.logbroker;

import java.util.List;

import lombok.Value;

import ru.yandex.kikimr.persqueue.consumer.StreamListener;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReadResponse;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;

@Value
public class LogbrokerRead {
    ConsumerReadResponse data;
    StreamListener.ReadResponder responder;

    public List<MessageBatch> getBatches() {
        return data.getBatches();
    }

    public long getCookie() {
        return data.getCookie();
    }
}
