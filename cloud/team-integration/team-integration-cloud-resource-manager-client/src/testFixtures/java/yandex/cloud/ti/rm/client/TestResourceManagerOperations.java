package yandex.cloud.ti.rm.client;

import java.util.concurrent.atomic.AtomicLong;

import org.jetbrains.annotations.NotNull;

public final class TestResourceManagerOperations {

    private static final @NotNull AtomicLong operationIdSequence = new AtomicLong();


    public static @NotNull String nextOperationId() {
        return templateOperationId(operationIdSequence.incrementAndGet());
    }

    private static @NotNull String templateOperationId(long cloudIdSeq) {
        return "resource-manager-operation-%d".formatted(
                cloudIdSeq
        );
    }


    private TestResourceManagerOperations() {
    }

}
