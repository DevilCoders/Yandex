package ru.yandex.monlib.metrics.encode.spack.compression;

import java.util.zip.Adler32;


/**
 * @author Sergey Polovko
 */
final class Adler32Checksum extends Adler32 implements Checksum {

    @Override
    public int calc(byte[] buffer, int offset, int length) {
        reset();
        update(buffer, offset, length);
        return (int) getValue();
    }
}
