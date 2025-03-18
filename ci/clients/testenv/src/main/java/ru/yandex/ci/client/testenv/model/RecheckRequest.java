package ru.yandex.ci.client.testenv.model;

import java.util.List;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class RecheckRequest {
    String checkId;
    String checkType;
    String taskId;
    int iterationNumber;
    int testsRetries;
    long svnRevision;
    String jobName;
    List<RecheckTarget> targets;

    @Value
    public static class RecheckTarget {
        String path;
        int partition;
        String toolchain;
    }
}
