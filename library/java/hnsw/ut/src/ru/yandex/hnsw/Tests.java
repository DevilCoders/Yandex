package ru.yandex.hnsw;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.util.Random;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import sun.nio.ch.DirectBuffer;

import ru.yandex.commune.offheap.MemoryUtils;
import ru.yandex.commune.offheap.buffers.simple.ByteOffheapBuffer;
import ru.yandex.misc.test.Assert;


/**
 * @author conterouz
 */
public class Tests {
    float[][] generate(int count, int dimension) {
        float[][] ff = new float[count][];
        Random r = new Random();
        for (int i = 0; i < count; i++) {
            ff[i] = new float[dimension];
            double len = 0;
            for (int j = 0; j < dimension; j++) {
                float value = r.nextFloat();
                ff[i][j] = value;
                len += value * value;
            }
            len = Math.sqrt(len);
            for (int j = 0; j < dimension; j++) {
                ff[i][j] = (float)(ff[i][j] / len);
            }
        }
        return ff;
    }

    float[][] generateDetemined(int count, int dimension, float seed) {
        float[][] ff = new float[count][];
        for (int i = 0; i < count; i++) {
            ff[i] = new float[dimension];
            double len = 0;
            for (int j = 0; j < dimension; j++) {
                float value = seed + i * dimension + j;
                ff[i][j] = value;
                len += value * value;
            }
            len = Math.sqrt(len);
            for (int j = 0; j < dimension; j++) {
                ff[i][j] = (float)(ff[i][j] / len);
            }
        }
        return ff;
    }

    ByteBuffer wrap(float[][] ff) {
        ByteBuffer bb = ByteBuffer.allocateDirect(ff.length * ff[0].length * Float.BYTES);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        for (int i = 0; i < ff.length; i++) {
            for (int j = 0; j < ff[i].length; j++) {
                bb.putFloat(ff[i][j]);
            }
        }
        return bb;
    }

    ByteOffheapBuffer wrapBOB(float[][] ff) {
        ByteOffheapBuffer bob = new ByteOffheapBuffer(ff.length * ff[0].length * Float.BYTES);
        for (int i = 0; i < ff.length; i++) {
            for (int j = 0; j < ff[i].length; j++) {
                MemoryUtils.putFloat(bob.getAddr() + i * j * Float.BYTES, ff[i][j]);
            }
        }
        return bob;
    }

    public static float[] unwrap(ByteBuffer bb, int pos, int dim) {
        float[] f = new float[dim];
        bb.position(getFloatVecOffset(pos, dim));
        for (int i = 0; i < dim; i++) {
            f[i] = bb.getFloat();
        }
        return f;
    }

    public static float[] unwrapBOB(ByteOffheapBuffer bob, int pos, int dim) {
        float[] f = new float[dim];
        for (int i = 0; i < dim; i++) {
            f[i] = MemoryUtils.getFloat(bob.getAddr() + pos * i * Float.BYTES);
        }
        return f;
    }

    public static String getVecStr(float[] fl) {
        StringBuilder sb = new StringBuilder();
        sb.append("(");
        for (int j = 0; j < fl.length; j++) {
            float value = fl[j];
            if (j > 0) {
                sb.append(", ");
            }
            sb.append(value);
            if (j > 5) {
                sb.append(" ...");
                break;
            }
        }
        sb.append(")");
        return sb.toString();
    }

    long getBufferAddress(ByteBuffer bb) {
        DirectBuffer db = (DirectBuffer)bb;
        return db.address();
    }

    public static IndexBuildOptions getOpts() {
        IndexBuildOptions opts = new IndexBuildOptions();
        opts.maxNeighbors = 2;
        opts.batchSize = 31;
        opts.upperLevelBatchSize = 63;
        opts.searchNeighborhoodSize = 15;
        opts.numExactCandidates = 17;
        opts.numThreads = 3;
        opts.levelSizeDecay = 2;
        opts.reportProgress = false;
        return opts;
    }

    public static int getFloatVecOffset(int pos, int dim) {
        return pos * dim * Float.BYTES;
    }

    public static void printResult(HNSWResultFloat nearestNeighbors, ByteBuffer storage, int dim) {
        for (int i =0; i < nearestNeighbors.indexes.length; i++) {
            System.out.println(
                    "\t" +
                            nearestNeighbors.indexes[i] +
                            getVecStr(unwrap(storage, nearestNeighbors.indexes[i], dim)) +
                            ", " +
                            nearestNeighbors.distances[i]);
        }
    }

    public static void printResultBOB(HNSWResultFloat nearestNeighbors, ByteOffheapBuffer storage, int dim) {
        for (int i =0; i < nearestNeighbors.indexes.length; i++) {
            System.out.println(
                    "\t" +
                            nearestNeighbors.indexes[i] +
                            getVecStr(unwrapBOB(storage, nearestNeighbors.indexes[i], dim)) +
                            ", " +
                            nearestNeighbors.distances[i]);
        }
    }

    static class Res {
        float[] vec;
        float dist;

        public Res(float[] vec, float dist) {
            this.vec = vec;
            this.dist = dist;
        }
    }

    Res findBest(float[][] ff, float[] vec) {
        int count = ff.length;
        int dim = ff[0].length;
        float max = 0;
        int maxi = 0;
        for (int i = 0; i < count; i++) {
            float dp = 0;
            for (int j = 0; j < dim; j++) {
                dp += vec[j] * ff[i][j];
            }
            if (dp > max) {
                max = dp;
                maxi = i;
            }
        }
        return new Res(ff[maxi], max);
    }

    File tmpFile;
    File tmpFile2;
    File tmpFile3index;
    File tmpFile3vectors;
    @Before
    public void setUp() throws IOException {
        tmpFile = File.createTempFile("hnsw_saveload_test", "");
        tmpFile2 = File.createTempFile("hnsw_saveload_test2", "");
        tmpFile3index = File.createTempFile("hnsw_saveload_test3_index", "");
        tmpFile3vectors = File.createTempFile("hnsw_saveload_test3_vectors", "");
    }
    @After
    public void tearDown() throws IOException {
        if (tmpFile != null) {
            Files.deleteIfExists(tmpFile.toPath());
        }
        if (tmpFile2 != null) {
            Files.deleteIfExists(tmpFile2.toPath());
        }
        if (tmpFile3index != null) {
            Files.deleteIfExists(tmpFile3index.toPath());
        }
        if (tmpFile3vectors != null) {
            Files.deleteIfExists(tmpFile3vectors.toPath());
        }
    }

    @Test
    public void simpleNoQuery() {
        final int count = 100;
        final int dim = 1;
        ByteBuffer bb  = wrap(generate(count, dim));
        new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
    }

    @Test
    public void simpleNoQueryDispose() {
        final int count = 100;
        final int dim = 1;
        ByteBuffer bb  = wrap(generate(count, dim));
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        ix.dispose();
    }

    public void simpleTest(final int count, final int dim) {
        float[][] ff = generate(count, dim);
        ByteBuffer bb  = wrap(ff);
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 56;
        float[] vec = unwrap(bb, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors, bb, dim);
        ix.dispose();
    }

    @Test
    public void twoDim() {
        simpleTest(1000, 2);
    }

    @Test
    public void tenDim() {
        simpleTest(5000, 100);
    }

    @Test
    public void saveLoad() throws IOException {
        final int count = 10000;
        final int dim = 10;
        float[][] ff = generate(count, dim);
        ByteBuffer bb  = wrap(ff);
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 56;
        float[] vec = unwrap(bb, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors, bb, dim);
        ix.save(tmpFile.getPath());
        ix.dispose();
        HNSWIndex ix2 = new HNSWIndex(tmpFile.getPath(), getBufferAddress(bb), count, dim);
        HNSWResultFloat nearestNeighbors2 = ix2.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors2, bb, dim);
        ix2.dispose();
        Assert.arraysEquals(nearestNeighbors.indexes, nearestNeighbors2.indexes);
        Assert.arraysEquals(nearestNeighbors.distances, nearestNeighbors2.distances);
    }

    @Test
    public void saveLoad2() throws IOException {
        final int count = 10000;
        final int dim = 10;
        float[][] ff = generate(count, dim);
        ByteBuffer bb  = wrap(ff);
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 56;
        float[] vec = unwrap(bb, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors, bb, dim);
        ix.save(tmpFile2.getPath());
        ix.dispose();

        File f = new File(tmpFile2.getPath());
        int length = (int) f.length();
        ByteBuffer bb2 = ByteBuffer.allocateDirect(length);
        FileChannel ch = new FileInputStream(f).getChannel();
        ch.read(bb2);
        HNSWIndex ix2 = new HNSWIndex(getBufferAddress(bb2), length, getBufferAddress(bb), count, dim);

        HNSWResultFloat nearestNeighbors2 = ix2.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors2, bb, dim);
        ix2.dispose();
        Assert.arraysEquals(nearestNeighbors.indexes, nearestNeighbors2.indexes);
        Assert.arraysEquals(nearestNeighbors.distances, nearestNeighbors2.distances);
    }

    @Test
    public void saveLoad3() throws IOException {
        final int count = 10000;
        final int dim = 10;
        float[][] ff = generate(count, dim);
        ByteBuffer bb  = wrap(ff);
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 56;
        float[] vec = unwrap(bb, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors, bb, dim);
        ix.save(tmpFile3index.getPath());
        ix.dispose();
        try (FileOutputStream fos = new FileOutputStream(tmpFile3vectors)) {
            bb.rewind();
            byte[] arr = new byte[bb.remaining()];
            bb.get(arr);
            fos.write(arr);
            bb.rewind();
        }
        HNSWIndex ix2 = new HNSWIndex(tmpFile3index.getPath(), tmpFile3vectors.getPath(), dim);
        HNSWResultFloat nearestNeighbors2 = ix2.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors2, bb, dim);
        ix2.dispose();
        Assert.arraysEquals(nearestNeighbors.indexes, nearestNeighbors2.indexes);
        Assert.arraysEquals(nearestNeighbors.distances, nearestNeighbors2.distances);
    }

    @Test
    public void saveLoad4() throws IOException {
        final int count = 10000;
        final int dim = 10;
        float[][] ff = generate(count, dim);
        ByteBuffer bb  = wrap(ff);
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 56;
        float[] vec = unwrap(bb, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors, bb, dim);

        long indexSize = ix.getIndexSize();
        ByteBuffer bb2 = ByteBuffer.allocateDirect((int) indexSize);
        ix.saveToMemory(getBufferAddress(bb2), indexSize);
        ix.dispose();

        HNSWIndex ix2 = new HNSWIndex(getBufferAddress(bb2), indexSize, getBufferAddress(bb), count, dim);
        HNSWResultFloat nearestNeighbors2 = ix2.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors2, bb, dim);
        ix2.dispose();

        Assert.arraysEquals(nearestNeighbors.indexes, nearestNeighbors2.indexes);
        Assert.arraysEquals(nearestNeighbors.distances, nearestNeighbors2.distances);
    }

    @Test
    public void simpleBOB() {
        final int count = 100;
        final int dim = 1;
        ByteOffheapBuffer bob = wrapBOB(generate(count, dim));
        new HNSWIndex(bob.getAddr(), count, dim, DistanceType.DOT_PRODUCT, getOpts());
    }

    public void simpleTestBOB(final int count, final int dim) {
        float[][] ff = generate(count, dim);
        ByteOffheapBuffer bob = wrapBOB(generate(count, dim));
        HNSWIndex ix = new HNSWIndex(bob.getAddr(), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 56;
        float[] vec = unwrapBOB(bob, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(bob.getAddr()+ getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResultBOB(nearestNeighbors, bob, dim);
        ix.dispose();
    }

    @Test
    public void testTwoDimBOB() {
        simpleTestBOB(1000, 2);
    }

    @Test
    public void simpleTestDeterminedArray() {
        final int count = 10000;
        final int dim = 10;
        float[][] ff = generateDetemined(count, dim, 0);
        ByteBuffer bb  = wrap(ff);
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 10;
        float[] vec = unwrap(bb, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(vec, dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors, bb, dim);
        Assert.assertEquals(nearestNeighbors.indexes.length, 1);
        Assert.assertEquals(nearestNeighbors.indexes[0], vecIx);
        ix.dispose();
    }

    @Test
    public void simpleTestDetermined() {
        final int count = 10000;
        final int dim = 10;
        float[][] ff = generateDetemined(count, dim, 0);
        ByteBuffer bb  = wrap(ff);
        HNSWIndex ix = new HNSWIndex(getBufferAddress(bb), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        final int vecIx = 10;
        float[] vec = unwrap(bb, vecIx, dim);
        System.out.println("Look for: " + getVecStr(vec));
        Res best = findBest(ff, vec);
        System.out.println("Best: " + getVecStr(best.vec) + ", " + best.dist);
        HNSWResultFloat nearestNeighbors = ix.getNearestNeighbors(getBufferAddress(bb) + getFloatVecOffset(vecIx, dim), dim, DistanceType.DOT_PRODUCT, 1, 10);
        printResult(nearestNeighbors, bb, dim);
        Assert.assertEquals(nearestNeighbors.indexes.length, 1);
        Assert.assertEquals(nearestNeighbors.indexes[0], vecIx);
        ix.dispose();
    }

}
