package ru.yandex.ci.flow.engine.runtime.state;

import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.util.jackson.SerializationUtils;

public class StageGroupTable extends KikimrTableCi<StageGroupState> {

    public StageGroupTable(QueryExecutor executor) {
        super(StageGroupState.class, executor);
    }

    public void initStage(String stageId) {
        if (findOptional(stageId).isEmpty()) {
            save(StageGroupState.of(stageId));
        }
    }

    public Optional<StageGroupState> findOptional(String id) {
        return find(StageGroupState.Id.of(id));
    }

    public Map<String, StageGroupState> findByIds(Set<String> ids) {
        var keys = ids.stream().map(StageGroupState.Id::of).collect(Collectors.toSet());
        return find(keys).stream()
                .collect(Collectors.toMap(stage -> stage.getId().getId(), Function.identity()));
    }

    public StageGroupState get(String id) {
        return get(StageGroupState.Id.of(id));
    }

    // *********************************************************************************
    // WARNING - объект не иммутабельный, если его поменять, то сохранить уже не удастся
    // *********************************************************************************
    @Nullable
    @Override
    protected StageGroupState findInternal(@Nonnull Entity.Id<StageGroupState> id) {
        var result = super.findInternal(id);
        return result == null ? result : SerializationUtils.copy(result, StageGroupState.class);
    }
}
