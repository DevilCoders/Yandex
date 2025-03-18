package ru.yandex.monlib.metrics.encode.spack.compression;

import net.jpountz.xxhash.XXHash32;
import net.jpountz.xxhash.XXHashFactory;


/**
 * @author Sergey Polovko
 */
class XxHash32Checksum implements Checksum {

    private static final int SEED = 0x1337c0de;

    private final XXHash32 impl = XXHashFactory.fastestInstance().hash32();

    @Override
    public int calc(byte[] buffer, int offset, int length) {
        return impl.hash(buffer, offset, length, SEED);
    }
}
