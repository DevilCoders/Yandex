package ru.yandex.ci.core.schema;

public class FakeDataFactory {

    public String nextString() {
        return "Monica";
    }

    public int nextInteger(int min, int maxInclusive) {
        return min + (maxInclusive - min) / 2;
    }

    public boolean nextBoolean() {
        return true;
    }
}
