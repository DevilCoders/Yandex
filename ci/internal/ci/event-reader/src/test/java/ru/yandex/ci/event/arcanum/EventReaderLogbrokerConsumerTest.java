package ru.yandex.ci.event.arcanum;

import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.arcanum.event.ArcanumEvents;
import ru.yandex.arcanum.event.ArcanumModels;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreStubConfig;
import ru.yandex.ci.event.spring.ArcanumServiceConfig;
import ru.yandex.ci.event.spring.CiEventReaderPropertiesConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.kikimr.persqueue.compression.CompressionCodec;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReadResponse;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageMeta;

@ContextConfiguration(classes = {
        ArcanumServiceConfig.class,
        BazingaCoreStubConfig.class,
        CiEventReaderPropertiesConfig.class
})
public class EventReaderLogbrokerConsumerTest extends YdbCiTestBase {
    @Mock
    private ConsumerReadResponse readResponse;

    @Mock
    private MessageBatch messageBatch;

    @Mock
    private MessageData messageData;

    @Mock
    private LbCommitCountdown lbCommitCountdown;

    @Autowired
    private ArcanumEventService arcanumEventService;

    @Test
    public void testArcanumEventsStreamListener() {
        ArcanumEvents.Event event = genEvent();
        Mockito.when(messageData.getMessageMeta()).thenReturn(
                new MessageMeta(new byte[0], 0, 0, 0, "", CompressionCodec.RAW, Map.of())
        );
        Mockito.when(messageData.getDecompressedData()).thenReturn(eventToBytes(event));
        List<MessageData> messageDataList = List.of(messageData);
        Mockito.when(messageBatch.getMessageData()).thenReturn(messageDataList);
        List<MessageBatch> messageBatches = List.of(messageBatch);

        var listener = new ArcanumReadProcessor(arcanumEventService);
        listener.onRead(
                new LogbrokerPartitionRead(
                        0,
                        lbCommitCountdown,
                        messageBatches
                )
        );
    }

    @Test
    public void testArcanumEventsStreamListenerException() {
        Mockito.when(messageData.getDecompressedData()).thenReturn(new byte[1]);
        List<MessageData> messageDataList = List.of(messageData);
        Mockito.when(messageBatch.getMessageData()).thenReturn(messageDataList);
        List<MessageBatch> messageBatches = List.of(messageBatch);
        Mockito.when(readResponse.getBatches()).thenReturn(messageBatches);

        ArcanumReadProcessor listener = new ArcanumReadProcessor(arcanumEventService);

        Assertions.assertThrows(RuntimeException.class, () ->
                listener.onRead(
                        new LogbrokerPartitionRead(
                                0,
                                lbCommitCountdown,
                                messageBatches
                        )
                ));
    }

    private ArcanumEvents.Event genEvent() {
        var statusEvent = ArcanumEvents.ReviewRequestStatusEvent.newBuilder()
                .setReviewRequest(
                        ArcanumModels.ReviewRequest.newBuilder()
                                .setId(123L)
                                .build()
                ).build();

        return ArcanumEvents.Event.newBuilder()
                .setReviewRequestOpened(statusEvent)
                .build();
    }

    private byte[] eventToBytes(ArcanumEvents.Event event) {
        return event.toByteArray();
    }
}
