package ru.yandex.ci.storage.reader.message;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.storage.core.message.ProcessingQueue;

class ProcessingQueueTest extends CommonTestBase {

    @Test
    void priority() throws Exception {
        ProcessingQueue<Element> queue = new ProcessingQueue<>();

        Element e1 = new Element(1);
        Element e2 = new Element(2);

        queue.add(e1);
        queue.add(e2);
        assertQueueSize(queue, 2, 0);

        var lock1 = queue.poll();
        var lock2 = queue.poll();
        assertQueueSize(queue, 0, 2);

        Assertions.assertThat(lock1.getValue()).isEqualTo(e1);
        Assertions.assertThat(lock2.getValue()).isEqualTo(e2);

        e1.priority = 42;
        e2.priority = 21;
        lock1.release();
        lock2.release();
        assertQueueSize(queue, 2, 0);

        lock1 = queue.poll();
        lock2 = queue.poll();
        assertQueueSize(queue, 0, 2);
        Assertions.assertThat(lock1.getValue()).isEqualTo(e2);
        Assertions.assertThat(lock2.getValue()).isEqualTo(e1);
    }

    @Test
    void skipEmpty() throws Exception {
        ProcessingQueue<Element> queue = new ProcessingQueue<>();

        Element e = new Element(true);
        queue.add(e);
        assertQueueSize(queue, 0, 0);

        e.empty = false;
        queue.add(e);
        assertQueueSize(queue, 1, 0);

        var lock = queue.poll();
        assertQueueSize(queue, 0, 1);

        e.empty = true;
        lock.release();
        assertQueueSize(queue, 0, 0);
    }

    @Test
    void deduplication() throws Exception {
        ProcessingQueue<Element> queue = new ProcessingQueue<>();
        Element e = new Element(1);

        queue.add(e);
        assertQueueSize(queue, 1, 0);

        e.priority = 2;
        queue.add(e);
        assertQueueSize(queue, 1, 0);

        var lock = queue.poll();
        assertQueueSize(queue, 0, 1);

        queue.add(e);
        assertQueueSize(queue, 0, 1);

        lock.release();
        assertQueueSize(queue, 1, 0);

        lock.release();
        assertQueueSize(queue, 1, 0);

    }

    private void assertQueueSize(ProcessingQueue<Element> queue, int enqueued, int locked) {
        Assertions.assertThat(queue.queueSize()).isEqualTo(enqueued);
        Assertions.assertThat(queue.lockedSize()).isEqualTo(locked);
    }

    private static class Element implements ProcessingQueue.QueueElement {
        private int priority = 0;
        private boolean empty = false;

        private Element(int priority) {
            this.priority = priority;
        }

        private Element(boolean empty) {
            this.empty = empty;
        }

        @Override
        public boolean isEmpty() {
            return empty;
        }

        @Override
        public long getPriority() {
            return priority;
        }
    }
}
