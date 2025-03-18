package ru.yandex.ci.engine.discovery.tier0;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskRequirements;
import ru.yandex.ci.client.sandbox.api.TaskSemaphoreAcquire;
import ru.yandex.ci.client.sandbox.api.TaskSemaphores;
import ru.yandex.ci.client.sandbox.api.notification.NotificationSetting;
import ru.yandex.ci.client.sandbox.api.notification.NotificationStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationTransport;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;

/**
 * GraphDiscoveryFakeTask sandbox task: https://nda.ya.ru/t/j9eFoXKK3mzBt3
 */
@Value
@Builder
class ChangesDetectorSandboxTaskBuilder {

    @Nonnull
    String type;
    @Nonnull
    String owner;
    @Nonnull
    SandboxTaskPriority priority;
    long leftSvnRevision;
    long rightSvnRevision;
    @Nonnull
    Set<GraphDiscoveryTask.Platform> platforms;
    boolean useDistbuildTestingCluster;
    @Nonnull
    String yaToken;
    @Nonnull
    ArcRevision rightRevision;
    @Nonnull
    ArcRevision leftRevision;
    @Nonnull
    String distbuildPool;

    SandboxTask asSandboxTask() {
        var tags = new ArrayList<String>();
        tags.add("CI");
        tags.add("GRAPH_DISCOVERY");

        var sortedPlatforms = new TreeSet<>(platforms);

        sortedPlatforms.stream()
                .map(Enum::name)
                .sorted()
                .forEach(tags::add);

        return SandboxTask.builder()
                .type(type)
                .owner(owner)
                .priority(priority)
                .requirements(new SandboxTaskRequirements()
                        .setSemaphores(
                                new TaskSemaphores().setAcquires(
                                        sortedPlatforms.stream()
                                                .map(source -> GraphDiscoveryHelper.platformToSandboxSemaphoreName(
                                                        source, useDistbuildTestingCluster
                                                ))
                                                .distinct()
                                                .map(semaphoreName -> new TaskSemaphoreAcquire()
                                                        .setName(semaphoreName)
                                                        .setWeight(1L)
                                                )
                                                .collect(Collectors.toList())
                                )
                        )
                )
                .description("right: %d (%s), left: %d (%s), %s".formatted(
                        rightSvnRevision, rightRevision,
                        leftSvnRevision, leftRevision,
                        sortedPlatforms
                ))
                .customFields(List.of(
                        new SandboxCustomField("build_profile", "pre_commit"),
                        new SandboxCustomField("targets", "autocheck"),
                        new SandboxCustomField("arc_url_right", buildArcUrl(rightRevision)),
                        new SandboxCustomField("arc_url_left", buildArcUrl(leftRevision)),
                        new SandboxCustomField("autocheck_yaml_url", buildArcUrl(rightRevision)),
                        new SandboxCustomField(
                                "platforms",
                                sortedPlatforms.stream()
                                        .map(GraphDiscoveryHelper::platformToSandboxPlatformParam)
                                        .collect(Collectors.toList())
                        ),
                        new SandboxCustomField("distbuild_pool", distbuildPool),
                        new SandboxCustomField("ya_token", yaToken),
                        new SandboxCustomField("compress_results", true)
                ))
                .tags(tags)
                .notifications(List.of(
                        new NotificationSetting(
                                NotificationTransport.EMAIL,
                                List.of(
                                        NotificationStatus.EXCEPTION,
                                        NotificationStatus.TIMEOUT
                                ),
                                // v-korovin is author of Sandbox task CHANGES_DETECTOR
                                List.of("v-korovin")
                        )
                ))
                .build();
    }

    private static String buildArcUrl(ArcRevision arcRevision) {
        return "arcadia-arc:/#" + arcRevision.getCommitId();
    }

    public static String platform(GraphDiscoveryTask.Platform it) {
        return switch (it) {
            case LINUX -> "linux";
            case MANDATORY -> "mandatory";
            case SANITIZERS -> "sanitizers";
            case GCC_MSVC_MUSL -> "gcc-msvc-musl";
            case IOS_ANDROID_CYGWIN -> "ios-android-cygwin";
        };
    }
}
