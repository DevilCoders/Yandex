package yandex.cloud.team.integration.abc;

import java.util.Optional;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;

/**
 * Service to deal with stub (proxy) operations.
 */
public interface StubOperationService {

    /**
     * Wraps main (real) operation with stub (proxy).
     *
     * @param mainOperation to wrap
     * @return stub operation
     */
    @NotNull Operation createStub(
            @NotNull Operation mainOperation
    );

    /**
     * Finds and updates (if necessary) stub operation. Does nothing for non-stub operations.
     *
     * @param operation to process
     * @return operation itself (for non-stub operations) or updated operation (for stub operations)
     */
    @NotNull Operation processStub(
            @NotNull Operation operation
    );

    /**
     * Returns existing create cloud operation, if any.
     *
     * @param abcId to create cloud
     * @return existing operation
     */
    Optional<Operation> findMainOperation(
            long abcId
    );

    /**
     * Saves main (original) cloud create operation.
     *
     * @param abcId ABC service id to create cloud for
     * @param mainOperation to save
     */
    void saveMainOperation(
            long abcId,
            @NotNull Operation mainOperation
    );

}
