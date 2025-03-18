package ru.yandex.ci.storage.core.check;

import java.util.Map;
import java.util.Set;

import lombok.Builder;
import lombok.Builder.Default;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;

@Value
@Builder(toBuilder = true)
public class CreateIterationParams {
    @Default
    IterationInfo info = IterationInfo.EMPTY;

    @Default
    Set<ExpectedTask> expectedTasks = Set.of();

    @Default
    String startedBy = "";

    boolean autorun;

    @Default
    Common.CheckTaskType tasksType = Common.CheckTaskType.CTT_AUTOCHECK;

    @Default
    boolean useImportantDiffIndex = true;

    @Default
    boolean useSuiteDiffIndex = true;

    @Default
    Map<Common.StorageAttribute, String> attributes = Map.of();

    long chunkShift;
}
