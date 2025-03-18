package ru.yandex.hnsw;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.Random;

import sun.nio.ch.DirectBuffer;

/**
 * @author conterouz
 */
public class Main {
    public static void main(String[] args) {
        //test(1000, 1);
        test(1000, 2);
        //test(1000, 2, true);
        //test(5);
        //test(500000, 100);
        //test(500000, 100, true);
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

    public static void test(int count, int dim) {
        test(count, dim, false);
    }
    public static void test(int count, int dim, boolean testSaveload) {

        System.out.println("Testing on " +count + " " + dim + " dimension vectors");
        final int size = count * dim;
        ByteBuffer bb = ByteBuffer.allocateDirect(size * Float.BYTES);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        DirectBuffer db = (DirectBuffer)bb;
        float[][] ff = new float[count][];
        Random r = new Random();
        for (int i = 0; i < count; i++) {
            ff[i] = new float[dim];
            for (int j = 0; j < dim; j++) {
                float value = r.nextFloat();
                bb.putFloat(value);
                ff[i][j] = value;
            }
        }



        HNSWIndex index = new HNSWIndex(db.address(), count, dim, DistanceType.DOT_PRODUCT, getOpts());
        int pos = 50;
        int offset =  (Float.BYTES * pos * dim);
        {
            System.out.println("Looking for: " + getVecStr(bb, pos, dim));
            HNSWResultFloat nearestNeighbors = index.getNearestNeighbors(db.address() + offset, dim, DistanceType.DOT_PRODUCT, 10, 10);
            printResult(nearestNeighbors, bb, dim);
        }

        if (testSaveload) {
            index.save("/tmp/idx");
            index.dispose();
        }

        if (testSaveload) {
            HNSWIndex index2 = new HNSWIndex("/tmp/idx", db.address(), count, dim);
            System.out.println("Looking for: " + getVecStr(bb, pos, dim));
            HNSWResultFloat nearestNeighbors = index2.getNearestNeighbors(db.address() + offset, dim, DistanceType.DOT_PRODUCT, 10, 10);
            printResult(nearestNeighbors, bb, dim);
        }

        if (testSaveload) {
            try {
                File f = new File("/tmp/idx");
                int length = (int) f.length();
                ByteBuffer ixbb = ByteBuffer.allocateDirect(length);
                DirectBuffer ixdb = (DirectBuffer)ixbb;
                FileChannel ch = new FileInputStream(f).getChannel();
                ch.read(ixbb);
                index = new HNSWIndex(ixdb.address(), length, db.address(), count, dim);
            } catch (IOException e) {
                e.printStackTrace();
            }
            System.out.println("Looking for: " + getVecStr(bb, pos, dim));
            HNSWResultFloat nearestNeighbors = index.getNearestNeighbors(db.address() + offset, dim, DistanceType.DOT_PRODUCT, 10, 10);
            printResult(nearestNeighbors, bb, dim);

        }

        float max = 0;
        int maxi = 0;
        float[] srch = ff[pos];
        for (int i = 0; i < count; i++) {
            float dp = 0;
            for (int j = 0; j < dim; j++) {
                dp += srch[j] * ff[i][j];
            }
            if (dp > max) {
                max = dp;
                maxi = i;
            }
        }
        System.out.println("brute: " + getVecStr(ff[maxi]) + ", dist: " + max);
        index.dispose();
        try {
            index.getNearestNeighbors(db.address() + offset, dim, DistanceType.DOT_PRODUCT, 4, 10);
        } catch (IllegalStateException e) {
            System.out.println("Dispose was OK");
        }
        System.out.println("-------\n");
    }


    public static void printResult(HNSWResultFloat nearestNeighbors, ByteBuffer storage, int dim) {
        for (int i =0; i < nearestNeighbors.indexes.length; i++) {
            System.out.println(
                    "\t" +
                    nearestNeighbors.indexes[i] +
                    getVecStr(storage, nearestNeighbors.indexes[i], dim) +
                    ", " +
                    nearestNeighbors.distances[i]);
        }
    }

    public static String getVecStr(ByteBuffer storage, int pos, int dim) {
        StringBuilder sb = new StringBuilder();
        sb.append("(");
        storage.position(4  * pos * dim);
        for (int j = 0; j < dim; j++) {
            float value = storage.getFloat();
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
}
