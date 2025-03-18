package ru.yandex.monlib.metrics.encode.spack.format;


import ru.yandex.monlib.metrics.encode.spack.SpackException;


/**
 * @author Sergey Polovko
 */
public enum TimePrecision {
    SECONDS((byte) 0x00),
    MILLIS((byte) 0x01),
    ;

    private static final TimePrecision[] VALUES = values();

    private final byte value;


    TimePrecision(byte value) {
        this.value = value;
    }

    public byte value() {
        return value;
    }

    public static TimePrecision valueOf(byte value) {
        for (TimePrecision timePrecision : VALUES) {
            if (timePrecision.value == value) {
                return timePrecision;
            }
        }
        throw new SpackException("unknown time precision value: " + value);
    }
}
