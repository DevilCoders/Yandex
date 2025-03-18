package ru.yandex.ci.core.config;

import java.time.Instant;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Value;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class ConfigCreationInfo {

    @JsonDeserialize(using = GsonJacksonDeserializer.class)
    Instant created;
    @Nullable
    OrderedArcRevision previousRevision;
    @Nullable
    OrderedArcRevision previousValidRevision;
    @Nullable
    OrderedArcRevision previousPullRequestRevision;

    public Optional<OrderedArcRevision> getPreviousRevision() {
        return Optional.ofNullable(previousRevision);
    }

    public Optional<OrderedArcRevision> getPreviousValidRevision() {
        return Optional.ofNullable(previousValidRevision);
    }

    public Optional<OrderedArcRevision> getPreviousPullRequestRevision() {
        return Optional.ofNullable(previousPullRequestRevision);
    }
}
