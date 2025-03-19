package yandex.cloud.ti.rm.client;

import java.time.Instant;
import java.util.concurrent.atomic.AtomicLong;

import com.google.protobuf.Timestamp;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.priv.resourcemanager.v1.PC;

public final class TestResourceManagerClouds {

    private static final @NotNull AtomicLong cloudIdSequence = new AtomicLong();


    public static @NotNull String nextCloudId() {
        return templateCloudId(cloudIdSequence.incrementAndGet());
    }

    private static @NotNull String templateCloudId(long cloudIdSeq) {
        return "cloud-%d".formatted(
                cloudIdSeq
        );
    }

    private static @NotNull String templateCloudName(@NotNull String cloudId) {
        return "%s name".formatted(
                cloudId
        );
    }

    private static @NotNull String templateCloudDescription(@NotNull String cloudId) {
        return "%s description".formatted(
                cloudId
        );
    }


    public static @NotNull Cloud nextCloud() {
        return nextCloud(
                null
        );
    }

    public static @NotNull Cloud nextCloud(
            @Nullable String organizationId
    ) {
        return templateCloud(
                nextCloudId(),
                organizationId
        );
    }

    public static @NotNull Cloud templateCloud(
            @NotNull String cloudId,
            @Nullable String organizationId
    ) {
        return new Cloud(
                cloudId,
                organizationId
        );
    }

    public static @NotNull PC.Cloud templateProtoCloud(
            @NotNull Cloud cloud
    ) {
        PC.Cloud.Builder builder = PC.Cloud.newBuilder()
                .setId(cloud.id());
        if (cloud.organizationId() != null) {
            builder.setOrganizationId(cloud.organizationId());
        }
        return builder
                .setName(templateCloudName(cloud.id()))
                .setDescription(templateCloudDescription(cloud.id()))
                .setCreatedAt(Timestamp.newBuilder()
                        .setSeconds(Instant.now().getEpochSecond())
                        .build()
                )
                .setStatus(PC.Cloud.Status.ACTIVE)
                .build();
    }


    private TestResourceManagerClouds() {
    }

}
