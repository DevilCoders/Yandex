package ru.yandex.hnsw;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import ru.yandex.misc.io.InputStreamSourceUtils;
import ru.yandex.misc.io.file.File2;
import ru.yandex.misc.io.file.FileNotFoundIoException;

/**
 * @author conterouz
 *
 */
public class HNSWIndex implements AutoCloseable {

    public static void loadLibraryFromClasspath(String classpath) {
        File2.withNewTempFile(tmpFile -> {
            InputStreamSourceUtils.valueOf(classpath).readTo(tmpFile);
            System.load(tmpFile.getAbsolutePath());
        });
    }

    static {
        // first try to load from system libraries or java.library.path
        try {
            System.loadLibrary("hnswjni");
        } catch (UnsatisfiedLinkError linkError) {
            // classpath:/ru/yandex/hnsw/jni/libhnswjni.so
            String classpath = "classpath:" + HNSWIndex.class.getPackage().getName().replace('.', '/') + "/jni/libhnswjni.so";
            try {
                loadLibraryFromClasspath(classpath);
            } catch (UnsatisfiedLinkError | FileNotFoundIoException linkError2) {
                throw new UnsatisfiedLinkError("Library libhnswjni expected in ${java.library.path} or as classpath resource at " + classpath);
            }
        }
    }

    private long indexAddress;

    public HNSWIndex(long vectorsAddress, int vectorsSize, int dimension, DistanceType distanceType, IndexBuildOptions options) {
        ObjectMapper mapper = new ObjectMapper();
        String jsonOptions;
        try {
            jsonOptions = mapper.writeValueAsString(options);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("Exception during writing IndexBuildOptions", e);
        }
        buildIndex(vectorsAddress, vectorsSize, dimension, distanceType, jsonOptions);
    }

    public HNSWIndex(long blobAddr, long blobSize, long vectorsStorageAddress, int vectorsCount, int dimension) {
        loadFromAddr(blobAddr, blobSize, vectorsStorageAddress, vectorsCount, dimension, false);
    }

    public HNSWIndex(String filename, long vectorsStorageAddress, int vectorsCount, int dimension) {
        loadFromFile(filename, vectorsStorageAddress, vectorsCount, dimension);
    }

    public HNSWIndex(String indexFilename, String vectorsFilename, int dimension) {
        loadFromFiles(indexFilename, vectorsFilename, dimension);
    }

    @Override
    public void close() {
        dispose();
    }

    public native HNSWResultFloat getNearestNeighbors(long queryAddress, int dimension, DistanceType distanceType, int topSize, int searchNeighbors);

    public native HNSWResultFloat getNearestNeighbors(float[] query, int dimension, DistanceType distanceType, int topSize, int searchNeighbors);

    // size in bytes needed to save hnsw index
    public native long getIndexSize();

    public native void saveToMemory(long blobAddr, long blobSize);

    public native void save(String filename);

    public native void dispose();

    private native void loadFromFile(String filename, long vectorsStorageAddress, int vectorsCount, int dimension);

    private native void loadFromFiles(String indexFilename, String vectorsFilename, int dimension);

    private native void loadFromAddr(long blobAddr, long blobSize, long vectorsStorageAddress, int vectorsCount, int dimension, boolean copyIndexMemory);

    private native void buildIndex(long vectorsAddress, int vectorsSize, int dimension, DistanceType distanceType, String jsonOptions);
}
