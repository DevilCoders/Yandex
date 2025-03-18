package ru.yandex.ci.logbroker;

import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

import lombok.ToString;

@ToString
public abstract class MessagesCountdown {
    private static final String ALL = "@all";

    private final int numberOfMessages;
    private final List<NestedMessageCountdown> nested;
    private final Map<String, AtomicInteger> stages;

    public MessagesCountdown(int numberOfMessages) {
        this.numberOfMessages = numberOfMessages;
        this.nested = new CopyOnWriteArrayList<>();
        this.stages = new ConcurrentHashMap<>();
        this.stages.put(ALL, new AtomicInteger(numberOfMessages));
    }

    public void notifyMessageProcessed() {
        this.notifyMessageProcessed(1);
    }

    public void notifyMessageProcessed(int size) {
        this.notifyMessageProcessed(ALL, size);
    }

    public void notifyMessageProcessed(String stageName) {
        this.notifyMessageProcessed(stageName, 1);
    }

    public void notifyMessageProcessed(String stageName, int size) {
        if (size == 0) {
            return;
        }

        var messagesLeft = this.stages.computeIfAbsent(stageName, s -> new AtomicInteger(this.numberOfMessages));
        int left = messagesLeft.addAndGet(-size);

        if (left < 0) {
            messagesLeft.addAndGet(size);
            onProcessedMoreThenHave(stageName, size);
        } else if (left == 0) {
            try {
                if (stageName.equals(ALL)) {
                    onAllMessagesProcessed();
                }
            } catch (RuntimeException e) {
                messagesLeft.addAndGet(size);
                throw new RuntimeException("Failed to notify all processed", e);
            }
        }
    }

    public int getNumberOfMessages() {
        return numberOfMessages;
    }

    public boolean areAllMessagesProcessed() {
        return stages.get(ALL).get() == 0;
    }

    protected abstract void onAllMessagesProcessed();

    protected void onProcessedMoreThenHave(String stageName, int size) {
        throw new RuntimeException(
                "Processed more message then have on stage %s, required: %d, stages: %s, nested: %s".formatted(
                        stageName, size, stages, nested
                )
        );
    }

    public MessagesCountdown createdNestedCountdown(int originalNumberOfMessages, int numberOfMessages) {
        var messagesCountdown = new NestedMessageCountdown(originalNumberOfMessages, numberOfMessages);
        this.nested.add(messagesCountdown);

        return messagesCountdown;
    }

    @ToString
    public class NestedMessageCountdown extends MessagesCountdown {
        private final int originalNumberOfMessages;

        public NestedMessageCountdown(int originalNumberOfMessages, int numberOfMessages) {
            super(numberOfMessages);
            this.originalNumberOfMessages = originalNumberOfMessages;
        }

        @Override
        protected void onAllMessagesProcessed() {
            MessagesCountdown.this.notifyMessageProcessed(originalNumberOfMessages);
        }
    }
}
