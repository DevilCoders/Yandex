package yandex.cloud.ti.abcd.provider;

import java.util.concurrent.atomic.AtomicLong;

import org.jetbrains.annotations.NotNull;

public final class TestAbcdProviders {

    private static final @NotNull AtomicLong sequence = new AtomicLong();
    private static final @NotNull AbcdProvider testAbcdProvider = nextAbcdProvider();


    public static @NotNull String nextAbcdProviderId() {
        return "abcd-provider-" + sequence.incrementAndGet();
    }

    public static @NotNull AbcdProvider nextAbcdProvider() {
        return new TestAbcdProvider(
                nextAbcdProviderId()
        );
    }

    public static @NotNull AbcdProvider testAbcdProvider() {
        return testAbcdProvider;
    }


    private TestAbcdProviders() {
    }

}
