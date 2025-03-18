package ru.yandex.ci.core.project;

import lombok.Value;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.timeline.TimelineBranchItem;

@Value(staticConstructor = "of")
public class ProcessIdBranch {
    CiProcessId processId;
    String branch;

    public static ProcessIdBranch of(TimelineBranchItem timelineBranchItem) {
        return ProcessIdBranch.of(timelineBranchItem.getProcessId(), timelineBranchItem.getId().getBranch());
    }
}
