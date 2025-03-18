package ru.yandex.ci.storage.tests.logbroker;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.Clock;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.protobuf.AbstractMessageLite;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.storage.core.MainStreamMessages;
import ru.yandex.ci.storage.core.message.main.MainStreamReadProcessor;
import ru.yandex.ci.storage.post_processor.logbroker.PostProcessorInReadProcessor;
import ru.yandex.ci.storage.reader.message.shard.ShardOutReadProcessor;
import ru.yandex.ci.storage.shard.message.ShardInReadProcessor;
import ru.yandex.kikimr.persqueue.compression.CompressionCodec;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageMeta;

@Slf4j
public class TestLogbrokerService {

    private final MessageQueue messageQueue = new MessageQueue();

    private final Clock clock;

    private final List<LogbrokerPartitionRead> processedReads = new ArrayList<>();
    private final AtomicInteger cookieCounter = new AtomicInteger(1);

    public TestLogbrokerService(Clock clock) {
        this.clock = clock;
    }

    public void registerMainConsumer(MainStreamReadProcessor mainStreamReadProcessor) {
        registerConsumer(StorageTopic.MAIN, mainStreamReadProcessor);
    }

    public void registerShardInConsumer(ShardInReadProcessor shardInProcessor) {
        registerConsumer(StorageTopic.SHARD_IN, shardInProcessor);
    }

    public void registerShardOutConsumer(ShardOutReadProcessor shardOutReadProcessor) {
        registerConsumer(StorageTopic.SHARD_OUT, shardOutReadProcessor);
    }

    public void registerEventsConsumer(LogbrokerReadProcessor logbrokerReadProcessor) {
        registerConsumer(StorageTopic.EVENTS, logbrokerReadProcessor);
    }

    public void registerPostProcessorInConsumer(PostProcessorInReadProcessor postProcessorInReadProcessor) {
        registerConsumer(StorageTopic.POST_PROCESSOR, postProcessorInReadProcessor);
    }

    public void registerConsumer(String topic, LogbrokerReadProcessor processorClass) {
        messageQueue.registerConsumer(topic, processorClass);
    }

    public LogbrokerPartitionRead createRead(List<byte[]> messages) {
        var messageMeta = new MessageMeta(
                "topic_writer".getBytes(StandardCharsets.UTF_8),
                0,
                0,
                0,
                "",
                CompressionCodec.RAW,
                Map.of()
        );

        var commitCountdown = new LbCommitCountdown(
                cookieCounter.getAndIncrement(),
                clock,
                () -> {
                },
                messages.size(),
                this::onReadProcessed
        );

        return new LogbrokerPartitionRead(
                0,
                commitCountdown,
                List.of(
                        new MessageBatch(
                                "topic",
                                0,
                                messages.stream()
                                        .map(x -> new MessageData(x, 0, messageMeta))
                                        .collect(Collectors.toList())
                        )
                )
        );
    }

    public void writeToMainStream(List<MainStreamMessages.MainStreamMessage> messages) {
        write(StorageTopic.MAIN, messages.stream().map(this::serialize).toList());
    }

    public void write(String topic, List<byte[]> messages) {
        messageQueue.add(topic, messages);
    }

    public void deliverAllMessages() {
        var hasNewMessages = true;
        while (hasNewMessages) {
            hasNewMessages = deliverAllMessagesInternal();
        }
    }

    @SuppressWarnings("ShortCircuitBoolean")
    protected boolean deliverAllMessagesInternal() {
        var hasNewMessages = deliverMainStreamMessages();

        /* use not lazy '|' instead of lazy `||` to reduce error possibility
            of swapping `hasNewMessages || deliverShardInMessages()` */
        hasNewMessages = deliverShardInMessages() | hasNewMessages;
        hasNewMessages = deliverShardOutMessages() | hasNewMessages;
        hasNewMessages = deliverEventsMessages() | hasNewMessages;
        hasNewMessages = deliverPostProcessorStreamMessages() | hasNewMessages;

        return hasNewMessages;
    }

    private boolean deliverEventsMessages() {
        return deliverStreamMessages(StorageTopic.EVENTS);
    }

    public boolean deliverShardOutMessages() {
        return deliverStreamMessages(StorageTopic.SHARD_OUT);
    }

    public boolean deliverShardInMessages() {
        return deliverStreamMessages(StorageTopic.SHARD_IN);
    }

    public boolean deliverMainStreamMessages() {
        return deliverStreamMessages(StorageTopic.MAIN);
    }

    public boolean deliverPostProcessorStreamMessages() {
        return deliverStreamMessages(StorageTopic.POST_PROCESSOR);
    }

    protected boolean deliverStreamMessages(String topic) {
        var hasNewMessages = false;

        for (var consumer : messageQueue.getTopicConsumers(topic)) {
            var queue = messageQueue.getQueue(consumer);
            if (!queue.isEmpty()) {
                hasNewMessages = true;

                var reads = readMessagesFromQueue(queue);
                this.processedReads.addAll(reads);

                messageQueue.getConsumer(consumer).process(reads);
            }
        }

        return hasNewMessages;
    }

    private List<LogbrokerPartitionRead> readMessagesFromQueue(BlockingQueue<MessageQueue.Batch> queue) {
        var messageBatches = new ArrayList<MessageQueue.Batch>(queue.size());
        queue.drainTo(messageBatches);

        return messageBatches.stream()
                .map(it -> createRead(it.getMessages()))
                .toList();
    }

    public void reset() {
        this.messageQueue.clear();
        this.processedReads.clear();
        this.cookieCounter.set(1);
    }

    private void onReadProcessed(StreamListener.ReadResponder responder, Long cookie) {
        log.info("Read processed, cookie {}", cookie);
    }

    public boolean areAllReadsCommited() {
        return this.processedReads.stream().allMatch(x -> x.getCommitCountdown().areAllMessagesProcessed());
    }

    public List<Long> getNotCommitedCookies() {
        return this.processedReads.stream()
                .filter(x -> !x.getCommitCountdown().areAllMessagesProcessed())
                .map(x -> x.getCommitCountdown().getCookie())
                .collect(Collectors.toList());
    }

    protected byte[] serialize(AbstractMessageLite<?, ?> message) {
        var output = new ByteArrayOutputStream();

        try {
            message.writeTo(output);
        } catch (IOException e) {
            throw new RuntimeException("", e);
        }

        return output.toByteArray();
    }

    @Value
    protected static class MessageQueue {

        Map<TopicAndConsumer, BlockingQueue<Batch>> messages = new HashMap<>();

        Map<TopicAndConsumer, LogbrokerReadProcessor> consumers = new HashMap<>();
        Map<String, Set<TopicAndConsumer>> consumersByTopic = new HashMap<>();

        public void registerConsumer(String topic, LogbrokerReadProcessor processor) {
            var key = TopicAndConsumer.of(topic, processor.getClass());
            consumers.put(key, processor);
            consumersByTopic.computeIfAbsent(topic, k -> new HashSet<>())
                    .add(key);
        }

        public void add(String topic, List<byte[]> messages) {
            for (var consumer : getTopicConsumers(topic)) {
                getQueue(consumer).add(Batch.of(messages));
            }
        }

        public Set<TopicAndConsumer> getTopicConsumers(String topic) {
            return consumersByTopic.getOrDefault(topic, Set.of());
        }

        public LogbrokerReadProcessor getConsumer(TopicAndConsumer consumer) {
            return consumers.get(consumer);
        }

        public void clear() {
            messages.clear();
        }

        private BlockingQueue<Batch> getQueue(TopicAndConsumer topicAndConsumer) {
            return messages.computeIfAbsent(topicAndConsumer, k -> new LinkedBlockingQueue<>());
        }

        @Value(staticConstructor = "of")
        private static class Batch {
            List<byte[]> messages;
        }

    }

    @Value
    private static class TopicAndConsumer {
        @Nonnull
        String topic;

        @Nonnull
        String consumer;

        static TopicAndConsumer of(String topic, Class<? extends LogbrokerReadProcessor> consumerClass) {
            return new TopicAndConsumer(topic, consumerClass.getCanonicalName());
        }
    }

}
