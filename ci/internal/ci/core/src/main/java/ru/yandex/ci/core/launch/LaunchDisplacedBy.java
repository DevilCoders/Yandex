package ru.yandex.ci.core.launch;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class LaunchDisplacedBy {
    @Nonnull
    Launch.Id id;

    @Nonnull
    Version version;
}
