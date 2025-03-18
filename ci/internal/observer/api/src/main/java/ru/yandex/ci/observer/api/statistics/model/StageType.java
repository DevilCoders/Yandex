package ru.yandex.ci.observer.api.statistics.model;

/**
 * ACTIVE - Only active stage of selected iteration (excluding finished stages and not started)
 * WITH_ZEROES - With all stages of selected iteration (including finished stages and not started (zero duration))
 * ANY - With all started stages of selected iteration (including finished stages, excluding not started)
 */
public enum StageType {
    ACTIVE("active"),
    WITH_ZEROES("with_zeroes"),
    ANY("any");

    private final String label;

    StageType(String label) {
        this.label = label;
    }

    @Override
    public String toString() {
        return label;
    }
}
