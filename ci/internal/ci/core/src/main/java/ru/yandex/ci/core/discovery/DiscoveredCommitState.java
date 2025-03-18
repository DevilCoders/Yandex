package ru.yandex.ci.core.discovery;

import java.time.Instant;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.versioning.Slot;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder(toBuilder = true)
@JsonSerialize(using = GsonJacksonSerializer.class)
@JsonDeserialize(using = GsonJacksonDeserializer.class)
public class DiscoveredCommitState {
    boolean dirDiscovery;
    boolean graphDiscovery;
    boolean pciDssDiscovery;
    ConfigChange configChange;
    @Singular
    List<LaunchId> launchIds;
    @Singular
    List<LaunchId> cancelledLaunchIds;
    @Singular
    List<LaunchId> displacedLaunchIds;
    List<Slot> versions;

    boolean manualDiscovery;
    @Nullable
    String manualDiscoveryBy;
    @Nullable
    Instant manualDiscoveryAt;

    @VisibleForTesting
    public LaunchId getLastLaunchId() {
        Preconditions.checkState(!launchIds.isEmpty());
        return launchIds.get(launchIds.size() - 1);
    }

    public List<LaunchId> getCancelledLaunchIds() {
        return Objects.requireNonNullElse(cancelledLaunchIds, List.of());
    }

    public List<LaunchId> getDisplacedLaunchIds() {
        return Objects.requireNonNullElse(displacedLaunchIds, List.of());
    }
}
