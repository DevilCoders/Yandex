package ru.yandex.hnsw;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * @author conterouz
 */

public class IndexBuildOptions {
    public final static int SIZE_AUTOSELECT = 0;

    @JsonProperty("max_neighbors")
    public int maxNeighbors;

    @JsonProperty("batch_size")
    public int batchSize;

    @JsonProperty("upper_level_batch_size")
    public int upperLevelBatchSize;

    @JsonProperty("search_neighborhood_size")
    public int searchNeighborhoodSize;

    @JsonProperty("num_exact_candidates")
    public int numExactCandidates;

    @JsonProperty("level_size_decay")
    public int levelSizeDecay;

    @JsonProperty("num_threads")
    public int numThreads;

    @JsonProperty("verbose")
    public boolean verbose;

    @JsonProperty("report_progress")
    public boolean reportProgress = true;

}


