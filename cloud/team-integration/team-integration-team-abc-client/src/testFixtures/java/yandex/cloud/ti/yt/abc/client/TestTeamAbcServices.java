package yandex.cloud.ti.yt.abc.client;

import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.atomic.AtomicLong;

import org.jetbrains.annotations.NotNull;

public final class TestTeamAbcServices {

    private static final @NotNull AtomicLong abcServiceIdSequence = new AtomicLong();


    public static long randomAbcServiceId() {
        return ThreadLocalRandom.current().nextLong(1, Integer.MAX_VALUE);
    }

    public static long nextAbcServiceId() {
        return abcServiceIdSequence.incrementAndGet();
    }

    public static @NotNull String templateAbcServiceSlug(long abcServiceId) {
        return "slug-%d".formatted(abcServiceId);
    }

    public static @NotNull String templateAbcServiceName(long abcServiceId) {
        return "name-%d".formatted(abcServiceId);
    }


    public static @NotNull TeamAbcService nextAbcService() {
        return templateAbcService(nextAbcServiceId());
    }

    public static @NotNull TeamAbcService templateAbcService(long abcServiceId) {
        return new TeamAbcService(
                abcServiceId,
                templateAbcServiceSlug(abcServiceId),
                templateAbcServiceName(abcServiceId)
        );
    }


    private TestTeamAbcServices() {
    }

}
