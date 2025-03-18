package ru.yandex.ci.storage.core.message;

import java.util.Deque;
import java.util.concurrent.ConcurrentLinkedDeque;

import javax.annotation.concurrent.ThreadSafe;

import com.google.common.base.Preconditions;

/**
 * Добавление в очередь происходить без блокировки
 * Для чтения берется блокировка на время парсинга
 */
@ThreadSafe
public class PartitionQueue implements ProcessingQueue.QueueElement {

    private final String partitionId;
    private final Deque<MessageContext> queue = new ConcurrentLinkedDeque<>();

    public PartitionQueue(String partitionId) {
        this.partitionId = partitionId;
    }

    public String getPartitionId() {
        return partitionId;
    }

    public void addMessage(MessageContext context) {
        queue.addLast(context);
    }

    public MessageContext pollMessage() {
        return queue.removeFirst();
    }

    public void returnMessage(MessageContext context) {
        queue.addFirst(context);
    }

    @Override
    public boolean isEmpty() {
        return queue.isEmpty();
    }

    @Override
    public long getPriority() {
        MessageContext context = queue.peekFirst();
        Preconditions.checkState(context != null);
        return context.getCountdown().getCookie();
    }
}
