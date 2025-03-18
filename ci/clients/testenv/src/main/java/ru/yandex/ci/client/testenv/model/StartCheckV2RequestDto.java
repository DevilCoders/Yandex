package ru.yandex.ci.client.testenv.model;

import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class StartCheckV2RequestDto {

    long diffSetId;
    int revision;

    @Nonnull
    String patch;
    @Nonnull
    String patchUrl;
    @Nonnull
    String owner;

    @Nonnull
    String gsid;
    @Nullable
    String branch;
    @Nonnull
    String commitMessage;
    @Nonnull
    List<String> diffPaths;

    boolean recheck;

    @Nullable
    String forceRecheckId;
    @Nullable
    List<String> fastTargets;
    @Nullable
    Boolean onlyFastCircuit;

    @Nullable
    String arcCommit;
    @Nullable
    String arcPrevCommit;

    @Nonnull
    CheckFastModeDto checkFastMode;
    boolean parallelRunFastAndFullCircuit;

    @Nullable
    String storageCheckId;

}
