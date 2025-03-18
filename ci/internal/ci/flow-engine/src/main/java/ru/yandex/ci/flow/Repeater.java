package ru.yandex.ci.flow;

import java.util.concurrent.TimeUnit;

public class Repeater {
    private final Runnable action;
    private final long timeoutMillis;
    private final long intervalMillis;

    Repeater(RepeaterBuilder builder) {
        this.action = builder.action;
        this.timeoutMillis = builder.timeoutMillis;
        this.intervalMillis = builder.intervalMillis;
    }

    public static RepeaterBuilder repeat(Runnable action) {
        return new RepeaterBuilder().repeat(action);
    }

    public void run() throws InterruptedException {
        long startTimeMillis = System.currentTimeMillis();

        while (true) {
            long elapsedTimeMillis = System.currentTimeMillis() - startTimeMillis;
            if (elapsedTimeMillis > timeoutMillis) {
                return;
            }

            action.run();

            TimeUnit.MILLISECONDS.sleep(intervalMillis);
        }
    }

    public static class RepeaterBuilder {
        private Runnable action;
        private long timeoutMillis = TimeUnit.MINUTES.toMillis(10);
        private long intervalMillis = TimeUnit.SECONDS.toMillis(4);

        RepeaterBuilder repeat(Runnable action) {
            this.action = action;
            return this;
        }

        public RepeaterBuilder timeout(long timeout, TimeUnit unit) {
            this.timeoutMillis = unit.toMillis(timeout);
            return this;
        }

        public RepeaterBuilder interval(long timeout, TimeUnit unit) {
            this.intervalMillis = unit.toMillis(timeout);
            return this;
        }

        public Repeater build() {
            return new Repeater(this);
        }

        public void run() throws InterruptedException {
            build().run();
        }
    }
}
