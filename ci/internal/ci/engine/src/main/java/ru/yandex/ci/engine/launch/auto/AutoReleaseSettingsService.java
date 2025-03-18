package ru.yandex.ci.engine.launch.auto;

import java.time.Clock;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class AutoReleaseSettingsService {

    private final CiMainDb db;
    private final Clock clock;

    public AutoReleaseSettingsService(CiMainDb db, Clock clock) {
        this.db = db;
        this.clock = clock;
    }

    @Nullable
    public AutoReleaseSettingsHistory findLastForProcessId(CiProcessId processId) {
        return db.currentOrReadOnly(() ->
                db.autoReleaseSettingsHistory().findLatest(processId)
        );
    }

    public Map<CiProcessId, AutoReleaseSettingsHistory> findLastForProcessIds(List<CiProcessId> processIds) {
        return db.currentOrReadOnly(() -> db.autoReleaseSettingsHistory().findLatest(processIds));
    }

    public void updateAutoReleaseState(CiProcessId processId, boolean enabled, String login, String message) {
        db.currentOrTx(() ->
                db.autoReleaseSettingsHistory().save(
                        AutoReleaseSettingsHistory.of(processId, enabled, login, message, clock.instant())
                )
        );
    }

}
