package ru.yandex.ci.common.ydb;

import java.lang.reflect.Proxy;
import java.util.concurrent.CompletableFuture;

import javax.annotation.Nonnull;

import com.yandex.ydb.core.Result;
import com.yandex.ydb.core.grpc.GrpcTransport;
import com.yandex.ydb.table.Session;
import com.yandex.ydb.table.query.DataQueryResult;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.client.YdbSessionManager;

@Slf4j
public class SessionManagerCI extends YdbSessionManager {

    public SessionManagerCI(@Nonnull KikimrConfig config, GrpcTransport transport) {
        super(config, transport);
    }

    @SuppressWarnings("unchecked")
    @Override
    public Session getSession() {
        var impl = super.getSession();
        return (Session) Proxy.newProxyInstance(getClass().getClassLoader(),
                new Class<?>[]{Session.class},
                (proxy, method, args) -> {
                    var result = method.invoke(impl, args);
                    if (method.getName().equals("executeDataQuery")) {
                        CompletableFuture<Result<DataQueryResult>> completableFuture =
                                (CompletableFuture<Result<DataQueryResult>>) result;
                        return completableFuture.thenApply(this::onResult);
                    } else {
                        return result;
                    }
                });
    }

    private Result<DataQueryResult> onResult(Result<DataQueryResult> result) {
        var issues = result.getIssues();
        for (var issue : issues) {
            log.info("YDB Issue: {}", issue);
        }
        return result;
    }
}
