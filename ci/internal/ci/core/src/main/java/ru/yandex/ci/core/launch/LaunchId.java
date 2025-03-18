package ru.yandex.ci.core.launch;

import lombok.Value;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.ydb.Persisted;

// TODO: Remove this class in favor of something else, CiProcessId cannot be proper deserialized
@Persisted
@Value(staticConstructor = "of")
public class LaunchId {
    CiProcessId processId;
    int number;

    public LaunchId(CiProcessId processId, int number) {
        this.processId = processId;
        this.number = number;
    }

    public Launch.Id toKey() {
        return Launch.Id.of(processId.asString(), number);
    }

    public static LaunchId fromKey(Launch.Id key) {
        return new LaunchId(CiProcessId.ofString(key.getProcessId()), key.getLaunchNumber());
    }
}
