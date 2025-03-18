package ru.yandex.ci.observer.core.db.model.settings;

import java.util.Set;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.storage.core.CheckIteration;

@Value
@Builder(toBuilder = true)
@Table(name = "ObserverSettings")
public class ObserverSettings implements Entity<ObserverSettings> {
    public static final Id ID = new Id("settings");

    public static final ObserverSettings EMTPY = ObserverSettings.builder()
            .id(ID)
            .processableIterationTypes(Set.of(CheckIteration.IterationType.FULL, CheckIteration.IterationType.FAST))
            .build();

    Id id;

    boolean stopAllRead;
    boolean skipMissingEntities;
    int fetchEntityRetryNumber;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nullable
    Set<CheckIteration.IterationType> processableIterationTypes;

    @Override
    public Id getId() {
        return id;
    }

    public Set<CheckIteration.IterationType> getProcessableIterationTypes() {
        return processableIterationTypes == null
                ? Set.of(CheckIteration.IterationType.FULL, CheckIteration.IterationType.FAST)
                : processableIterationTypes;
    }

    @Value
    public static class Id implements Entity.Id<ObserverSettings> {
        String id;
    }
}
