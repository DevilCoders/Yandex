package ru.yandex.monlib.metrics.encode.spack;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


/**
 * @author Sergey Polovko
 */
public class StringPool {

    private final String[] pool;

    public StringPool(byte[] in, int len) {
        List<String> indexedPool = new ArrayList<>(127);

        int startIdx = 0;
        while (startIdx < len) {
            // find NUL-byte
            int endIdx = startIdx;
            while (endIdx < len && in[endIdx] != 0) {
                endIdx++;
            }

            ByteBuffer inBuf = ByteBuffer.wrap(in, startIdx, endIdx - startIdx);
            CharBuffer charBuf = StandardCharsets.UTF_8.decode(inBuf);
            indexedPool.add(charBuf.toString());
            startIdx = endIdx + 1; // skip NUL-byte
        }

        this.pool = indexedPool.toArray(new String[indexedPool.size()]);
    }

    public String get(int index) {
        return pool[index];
    }

    public int size() {
        return pool.length;
    }

    @Override
    public String toString() {
        return "StringPool" + Arrays.toString(pool);
    }
}
