package yandex.cloud.ti.abcd.adapter;

import java.util.Map;
import java.util.function.BiConsumer;
import java.util.function.Function;

import io.grpc.Context;
import io.grpc.stub.StreamObserver;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.ti.abc.repo.ListPage;

import ru.yandex.intranet.d.backend.service.provider_proto.Account;
import ru.yandex.intranet.d.backend.service.provider_proto.AccountsServiceGrpc;
import ru.yandex.intranet.d.backend.service.provider_proto.CreateAccountRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.GetAccountRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsByFolderRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsByFolderResponse;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsResponse;
import ru.yandex.intranet.d.backend.service.provider_proto.UpdateProvisionRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.UpdateProvisionResponse;

class AccountsServiceGrpcImpl extends AccountsServiceGrpc.AccountsServiceImplBase {

    private final @NotNull AccountServiceFacade accountServiceFacade;
    private final @NotNull GrpcCallHandler callHandler;


    AccountsServiceGrpcImpl(
            @NotNull AccountServiceFacade accountServiceFacade,
            @NotNull GrpcCallHandler callHandler
    ) {
        this.accountServiceFacade = accountServiceFacade;
        this.callHandler = callHandler;
    }


    @Override
    public void createAccount(
            @NotNull CreateAccountRequest request,
            @NotNull StreamObserver<Account> responseObserver
    ) {
        runWithIdempotencyKey(
                request, responseObserver,
                CreateAccountRequest::getOperationId,
                this::createAccountIdempotent
        );
    }

    private void createAccountIdempotent(
            @NotNull CreateAccountRequest request,
            @NotNull StreamObserver<Account> responseObserver
    ) {
        callHandler.invoke(responseObserver, () -> {
            var parameters = AccountsServiceRequests.toCreateAccountParameters(request);
            AbcdAccount result = accountServiceFacade.createAccount(parameters);
            return AccountsServiceResponses.toAccount(result);
        });
    }

    @Override
    public void getAccount(
            @NotNull GetAccountRequest request,
            @NotNull StreamObserver<Account> responseObserver
    ) {
        callHandler.invoke(responseObserver, () -> {
            var parameters = AccountsServiceRequests.toGetAccountParameters(request);
            AbcdAccount result = accountServiceFacade.getAccount(parameters);
            return AccountsServiceResponses.toAccount(result);
        });
    }

    @Override
    public void listAccounts(
            @NotNull ListAccountsRequest request,
            @NotNull StreamObserver<ListAccountsResponse> responseObserver
    ) {
        callHandler.invoke(responseObserver, () -> {
            var parameters = AccountsServiceRequests.toListAccountsParameters(request);
            ListPage<AbcdAccount> result = accountServiceFacade.listAccounts(parameters);
            return AccountsServiceResponses.toListAccountsResponse(result);
        });
    }

    @Override
    public void listAccountsByFolder(
            @NotNull ListAccountsByFolderRequest request,
            @NotNull StreamObserver<ListAccountsByFolderResponse> responseObserver
    ) {
        callHandler.invoke(responseObserver, () -> {
            var parameters = AccountsServiceRequests.toListAccountsParameters(request);
            ListPage<AbcdAccount> result = accountServiceFacade.listAccounts(parameters);
            return AccountsServiceResponses.toListAccountsByFolderResponse(result);
        });
    }

    @Override
    public void updateProvision(
            @NotNull UpdateProvisionRequest request,
            @NotNull StreamObserver<UpdateProvisionResponse> responseObserver
    ) {
        runWithIdempotencyKey(
                request, responseObserver,
                UpdateProvisionRequest::getOperationId,
                this::updateProvisionIdempotent
        );
    }

    private void updateProvisionIdempotent(
            @NotNull UpdateProvisionRequest request,
            @NotNull StreamObserver<UpdateProvisionResponse> responseObserver
    ) {
        callHandler.invoke(responseObserver, () -> {
            var parameters = AccountsServiceRequests.toUpdateProvisionParameters(request);
            AbcdAccount result = accountServiceFacade.updateProvision(parameters);
            return AccountsServiceResponses.toUpdateProvisionResponse(result);
        });
    }

    private static <Req, Resp> void runWithIdempotencyKey(
            @NotNull Req request,
            @NotNull StreamObserver<Resp> responseObserver,
            @NotNull Function<Req, String> idempotencyKeyExtractor,
            @NotNull BiConsumer<? super Req, ? super StreamObserver<Resp>> consumer
    ) {
        String idempotencyKey = idempotencyKeyExtractor.apply(request);
        if (idempotencyKey == null || idempotencyKey.isEmpty()) {
            consumer.accept(request, responseObserver);
        } else {
            Context context = GrpcHeaders.createContext(Map.of(GrpcHeaders.IDEMPOTENCY_KEY, idempotencyKey));
            context.run(() -> consumer.accept(request, responseObserver));
        }
    }

}
