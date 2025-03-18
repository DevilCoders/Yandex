package ru.yandex.hnsw;

/**
 * @author conterouz
 */
public class HNSWResultFloat {
    public final int[] indexes;
    public final float[] distances;

    public HNSWResultFloat(int[] indexes, float[] distances) {
        this.indexes = indexes;
        this.distances = distances;
    }
}
