package ru.yandex.monlib.metrics;

/**
 * @author Sergey Polovko
 */
public class MetricConsumerState {

    public enum State {
        NOT_STARTED,
        ROOT,
        COMMON_LABELS,
        METRICS_ARRAY,
        METRIC,
        METRIC_LABELS,
        ENDED,
    }

    private State state = State.NOT_STARTED;

    public State get() {
        return state;
    }

    public void set(State state) {
        this.state = state;
    }

    public void expect(State state) {
        if (state != this.state) {
            throw new IllegalStateException("expected state " + state + ", but was " + this.state);
        }
    }

    public void swap(State expected, State newState) {
        expect(expected);
        state = newState;
    }

    public void throwUnexpected() {
        throw new IllegalStateException("unexpected state: " + state);
    }
}
