package ru.yandex.ci.core.launch;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder(toBuilder = true)
@SuppressWarnings("ReferenceEquality")
public class ReleaseVcsInfo {
    /**
     * Ревизия предыдущего релиза, который завершен успешно.
     * Неизменна на протяжении всего времени жизни релиза.
     */
    @Nullable
    OrderedArcRevision stableRevision;
    /**
     * Ревизия предыдущего неотмененного релиза.
     * Если предыдущий релиз отменится раньше, чем текущий завершит работу, то значение этого поля изменится.
     */
    @Nullable
    @With
    OrderedArcRevision previousRevision;
}
