package ru.yandex.monlib.metrics.encode.spack.format;

import javax.annotation.Nullable;

/**
 * @author Sergey Polovko
 */
public enum SpackVersion {
    // msb - major, lsb - minor
    v1_0(0x0100),
    v1_1(0x0101),
    v1_2(0x0102),
    ;

    public static final SpackVersion CURRENT = SpackVersion.v1_1;
    public static final SpackVersion LATEST = SpackVersion.v1_2;

    public static final SpackVersion[] ALL = values();

    private final short value;

    SpackVersion(int value) {
        this.value = (short) value;
    }

    public short getValue() {
        return value;
    }

    public boolean lt(SpackVersion rhs) {
        return value < rhs.value;
    }

    public boolean le(SpackVersion rhs) {
        return value <= rhs.value;
    }

    public boolean gt(SpackVersion rhs) {
        return value > rhs.value;
    }

    public boolean ge(SpackVersion rhs) {
        return value >= rhs.value;
    }

    @Nullable
    public static SpackVersion byValue(int value) {
        if (value >= v1_0.value && value <= LATEST.value) {
            return ALL[value & 0xff];
        }
        return null;
    }
}
