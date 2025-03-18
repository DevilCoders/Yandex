package ru.yandex.ci.engine.launch.version;

import lombok.Value;

import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.launch.versioning.Versions;

@Value(staticConstructor = "of")
class NextVersion {
    Versions versions;
    Version next;
}
