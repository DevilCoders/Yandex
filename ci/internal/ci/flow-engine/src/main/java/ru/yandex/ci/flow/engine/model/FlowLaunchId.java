package ru.yandex.ci.flow.engine.model;

import java.nio.charset.StandardCharsets;

import javax.annotation.Nonnull;

import com.google.common.hash.Hasher;
import com.google.common.hash.Hashing;
import lombok.AccessLevel;
import lombok.Getter;
import lombok.Value;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Persisted
@Getter(AccessLevel.NONE)
@Value(staticConstructor = "of")
@BenderBindAllFields
public class FlowLaunchId {
    @Nonnull
    String value;

    public static FlowLaunchId of(LaunchId launchId) {
        Hasher hasher = Hashing.sha256().newHasher();
        hasher.putString(launchId.getProcessId().asString(), StandardCharsets.UTF_8);
        hasher.putInt(launchId.getNumber());

        return FlowLaunchId.of(hasher.hash().toString());
    }

    public String asString() {
        return value;
    }

}
