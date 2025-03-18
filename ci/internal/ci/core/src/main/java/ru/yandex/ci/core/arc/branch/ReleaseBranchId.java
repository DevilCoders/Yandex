package ru.yandex.ci.core.arc.branch;

import lombok.Value;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;

@Value(staticConstructor = "of")
public class ReleaseBranchId {
    CiProcessId processId;
    ArcBranch branch;
}
