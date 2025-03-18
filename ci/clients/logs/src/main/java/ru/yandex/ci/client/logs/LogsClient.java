package ru.yandex.ci.client.logs;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.function.Supplier;

import com.google.common.base.Preconditions;
import one.util.streamex.StreamEx;
import vtail.api.core.Log;
import vtail.api.query.Api;
import vtail.api.query.QueryGrpc;

import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.common.proto.ProtoConverter;

public class LogsClient implements AutoCloseable {
    private final GrpcClient grpcClient;
    private final Supplier<QueryGrpc.QueryBlockingStub> queryStub;

    private LogsClient(GrpcClientProperties properties) {
        this.grpcClient = GrpcClientImpl.builder(properties, getClass())
                .excludeLoggingFullResponse(QueryGrpc.getFetchLogsMethod())
                .build();
        this.queryStub = grpcClient.buildStub(QueryGrpc::newBlockingStub);
    }

    public static LogsClient create(GrpcClientProperties properties) {
        return new LogsClient(properties);
    }

    public List<Log.LogMessage> getLog(Instant from, Instant to, Map<String, String> query, int limit) {
        Preconditions.checkArgument(limit > 0 && limit <= 10000, "limit should between in (0, 10000]");

        var queryString = StreamEx.of(query.entrySet())
                .sorted(Map.Entry.comparingByKey())
                .map(e -> e.getKey() + "=" + e.getValue())
                .joining(" ");

        var iterator = queryStub.get()
                .fetchLogs(
                        Api.FetchLogsRequest.newBuilder()
                                .setStart(ProtoConverter.convert(from))
                                .setEnd(ProtoConverter.convert(to))
                                .setQuery(queryString)
                                .setLimit(10000)
                                .setSortDir(Api.Sort.DESC)
                                .build()
                );
        return StreamEx.of(iterator).map(Api.FetchLogsRecord::getMsg).toList();
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }
}
