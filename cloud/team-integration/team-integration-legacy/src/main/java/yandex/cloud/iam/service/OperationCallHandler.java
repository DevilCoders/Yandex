package yandex.cloud.iam.service;

import java.util.Optional;
import java.util.function.Supplier;

import lombok.AccessLevel;
import lombok.NonNull;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.experimental.UtilityClass;
import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.model.operation.OperationContext;

@UtilityClass
// should be migrated to OperationCallHandler2
@Deprecated
public class OperationCallHandler {

    public static <T> Supplier<OperationEither<T>> getIdempotentOperationOrRun(@NonNull Supplier<T> supplier) {
        return () -> Optional.ofNullable(GrpcHeaders.getIdempotencyKey())
                .map(OperationContext::findByIdempotencyKey)
                .map(OperationEither::<T>operation)
                .orElseGet(() -> OperationEither.value(supplier.get()));
    }

    public static Supplier<Operation> withIdempotence(@NonNull Supplier<Operation> supplier) {
        return () -> Optional.ofNullable(GrpcHeaders.getIdempotencyKey())
                .map(OperationContext::findByIdempotencyKey)
                .orElseGet(() -> invokeAndSave(supplier));
    }

    private static Operation invokeAndSave(Supplier<Operation> supplier) {
        var operation = supplier.get();
        if (GrpcHeaders.getIdempotencyKey() != null) {
            OperationContext.createIdempotentOperation(operation.getId(), GrpcHeaders.getIdempotencyKey());
        }

        return operation;
    }

    public interface OperationEither<T> {
        static <T> OperationEither<T> value(@NonNull T value) {
            return IsValue.of(value);
        }

        static <T> OperationEither<T> operation(@NonNull Operation operation) {
            return IsOperation.of(operation);
        }

        boolean isOperation();

        Operation getOperation();

        boolean isValue();

        T getValue();

        @RequiredArgsConstructor(access = AccessLevel.PRIVATE, staticName = "of")
        @Value
        class IsOperation<T> implements OperationEither<T> {
            Operation operation;

            @Override
            public boolean isOperation() {
                return true;
            }

            @Override
            public boolean isValue() {
                return false;
            }

            @Override
            public T getValue() {
                throw new IllegalStateException("OperationEither<> does not contain value.");
            }
        }

        @RequiredArgsConstructor(access = AccessLevel.PRIVATE, staticName = "of")
        @Value
        class IsValue<T> implements OperationEither<T> {
            T value;

            @Override
            public boolean isOperation() {
                return false;
            }

            @Override
            public Operation getOperation() {
                throw new IllegalStateException("OperationEither<> does not contain operation.");
            }

            @Override
            public boolean isValue() {
                return true;
            }
        }
    }
}
