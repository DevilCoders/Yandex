package ru.yandex.ci.storage.core.message;

import java.util.HashSet;
import java.util.PriorityQueue;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

import javax.annotation.concurrent.ThreadSafe;

import com.google.common.base.Preconditions;


/**
 * <ul>
 * Очередь обработки со следующими свойствами:
 * <li>Элементы дедуплицируются по equals
 * <li>Элемент можно взять из очереди только однажды. В этот моментна него берется лок.
 * Он будет недоступен пока его явно не вернут обратно, через метод release()
 * <li>Элемент добавляется (и возвращается) в очередь только если он !isEmpty().
 * В случае если он isEmpty() его можно заново добавить через add()
 * <li>У элементов должен быть приоритет. Значение приоритета учитывается на момент добавления(возвращение) в очередь.
 * Элементы с меньшим приоритетом будут возвращаться из очереди раньше.
 * </ul>
 */
@ThreadSafe
public class ProcessingQueue<E extends ProcessingQueue.QueueElement> {

    private final ReentrantLock lock = new ReentrantLock();
    private final Condition notEmpty = lock.newCondition();

    private final HashSet<E> locked = new HashSet<>();
    private final HashSet<E> enqueued = new HashSet<>();
    private final PriorityQueue<InternalQueueElement> queue = new PriorityQueue<>();

    public void add(E e) {
        lock.lock();
        try {
            addInternal(e);
        } finally {
            lock.unlock();
        }
    }

    public QueueLock poll() throws InterruptedException {
        E e;
        lock.lock();
        try {
            while (queue.isEmpty()) {
                notEmpty.await();
            }
            e = pollInternal();
        } finally {
            lock.unlock();
        }
        return new QueueLock(e);
    }

    public int queueSize() {
        lock.lock();
        try {
            return queue.size();
        } finally {
            lock.unlock();
        }
    }

    public int lockedSize() {
        lock.lock();
        try {
            return locked.size();
        } finally {
            lock.unlock();
        }
    }

    private E pollInternal() {
        var internalElement = queue.poll();
        Preconditions.checkNotNull(internalElement);
        E e = internalElement.element;
        enqueued.remove(e);
        locked.add(e);
        return e;
    }

    private void addInternal(E e) {
        if (!locked.contains(e) && !e.isEmpty() && !enqueued.contains(e)) {
            queue.add(new InternalQueueElement(e));
            enqueued.add(e);
            notEmpty.signal();
        }
    }

    @Override
    public String toString() {
        lock.lock();
        try {
            return "{queueSize=" + queue.size() + ", processingSize=" + locked.size() + "}";
        } finally {
            lock.unlock();
        }
    }

    public interface QueueElement {
        boolean isEmpty();

        /**
         * less if better
         * Should not be called if element isEmpty
         */
        long getPriority();
    }

    private class InternalQueueElement implements Comparable<InternalQueueElement> {
        private final long priority;
        private final E element;

        private InternalQueueElement(E element) {
            this.element = element;
            //Saving priority cause it can change dynamically
            this.priority = element.getPriority();
        }

        @Override
        public int compareTo(ProcessingQueue<E>.InternalQueueElement o) {
            return Long.compare(priority, o.priority);
        }
    }

    private void unlock(QueueLock element) {
        if (!element.acquired) {
            return;
        }

        var value = element.value;

        this.lock.lock();
        try {
            if (element.acquired) {
                this.locked.remove(value);
                element.acquired = false;
                addInternal(value);
            }
        } finally {
            this.lock.unlock();
        }
    }

    public class QueueLock {
        private final E value;
        private volatile boolean acquired;

        public QueueLock(E value) {
            this.value = value;
            this.acquired = true;
        }

        public E getValue() {
            return value;
        }

        public void release() {
            ProcessingQueue.this.unlock(this);
        }
    }
}

