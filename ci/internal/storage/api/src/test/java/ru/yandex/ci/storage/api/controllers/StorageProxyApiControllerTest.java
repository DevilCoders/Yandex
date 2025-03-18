package ru.yandex.ci.storage.api.controllers;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.storage.api.ApiTestBase;
import ru.yandex.ci.storage.api.StorageProxyApi;
import ru.yandex.ci.storage.api.StorageProxyApi.WriteMessagesRequest;
import ru.yandex.ci.storage.api.StorageProxyApiServiceGrpc;
import ru.yandex.ci.storage.api.StorageProxyApiServiceGrpc.StorageProxyApiServiceBlockingStub;
import ru.yandex.ci.storage.api.proxy.StorageMessageProxyWriter;
import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.MainStreamMessages.MainStreamMessage;
import ru.yandex.ci.storage.core.TaskMessages.TaskMessage;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

@ExtendWith(MockitoExtension.class)
class StorageProxyApiControllerTest extends ApiTestBase {

    @Mock
    private StorageMessageProxyWriter trunkPreCommit;

    @Mock
    private StorageMessageProxyWriter trunkPostCommit;

    @Mock
    private StorageMessageProxyWriter branchPreCommit;

    @Mock
    private StorageMessageProxyWriter branchPostCommit;

    private StorageProxyApiServiceBlockingStub stub;

    @BeforeEach
    void init() {
        var controller = new StorageProxyApiController(Map.of(
                CheckType.TRUNK_PRE_COMMIT, trunkPreCommit,
                CheckType.TRUNK_POST_COMMIT, trunkPostCommit,
                CheckType.BRANCH_PRE_COMMIT, branchPreCommit,
                CheckType.BRANCH_POST_COMMIT, branchPostCommit));
        stub = StorageProxyApiServiceGrpc.newBlockingStub(buildChannel(controller));
    }

    @AfterEach
    void verifyNoInvocations() {
        verifyNoMoreInteractions(trunkPreCommit);
        verifyNoMoreInteractions(trunkPostCommit);
        verifyNoMoreInteractions(branchPreCommit);
        verifyNoMoreInteractions(branchPostCommit);
    }

    @Test
    void writeMessagesEmpty() {
        this.sendRequest(WriteMessagesRequest.getDefaultInstance());
    }

    @ParameterizedTest
    @MethodSource("requests")
    void writeMessage(WriteMessagesRequest request) {
        this.sendRequest(request);
        var mock = switch (request.getCheckType()) {
            case TRUNK_PRE_COMMIT -> trunkPreCommit;
            case TRUNK_POST_COMMIT -> trunkPostCommit;
            case BRANCH_PRE_COMMIT -> branchPreCommit;
            case BRANCH_POST_COMMIT -> branchPostCommit;
            case UNRECOGNIZED -> null;
        };

        if (mock != null) {
            var messages = request.getMessagesList();
            verify(mock).writeTasks(messages);
        }
    }

    private void sendRequest(WriteMessagesRequest request) {
        var response = stub.writeMessages(request);
        assertThat(response).isEqualTo(StorageProxyApi.WriteMessagesResponse.getDefaultInstance());
    }

    static List<WriteMessagesRequest> requests() {
        return Stream.of(CheckType.values())
                .filter(type -> type != CheckType.UNRECOGNIZED)
                .map(type -> WriteMessagesRequest.newBuilder()
                        .setCheckType(type)
                        .addMessages(MainStreamMessage.newBuilder().setTaskMessage(
                                TaskMessage.newBuilder()
                                        .setPartition(1)
                                        .setPessimize(Actions.Pessimize.newBuilder()
                                                .setInfo("test-" + type) // just for test
                                                .build())
                                        .build()))
                        .build())
                .collect(Collectors.toList());
    }
}
