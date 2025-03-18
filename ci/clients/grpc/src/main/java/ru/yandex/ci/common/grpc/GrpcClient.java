package ru.yandex.ci.common.grpc;

import java.util.function.Function;
import java.util.function.Supplier;

import io.grpc.ManagedChannel;
import io.grpc.stub.AbstractStub;

public interface GrpcClient extends AutoCloseable {
    <T extends AbstractStub<T>> Supplier<T> buildStub(Function<ManagedChannel, T> constructor);
}
