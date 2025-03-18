package ru.yandex.ci.ayamler.api.controllers;

import java.nio.file.Path;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import io.grpc.Context;
import io.grpc.stub.StreamObserver;
import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Histogram;

import ru.yandex.ci.ayamler.AYaml;
import ru.yandex.ci.ayamler.AYamlerApiMetrics;
import ru.yandex.ci.ayamler.AYamlerService;
import ru.yandex.ci.ayamler.AYamlerServiceGrpc;
import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.ayamler.Ayamler.GetAbcServiceSlugBatchRequest;
import ru.yandex.ci.ayamler.Ayamler.GetAbcServiceSlugBatchResponse;
import ru.yandex.ci.ayamler.Ayamler.GetAbcServiceSlugRequest;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeBatchRequest;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeBatchResponse;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeRequest;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeResponse;
import ru.yandex.ci.ayamler.PathAndLogin;
import ru.yandex.ci.ayamler.PathNotFoundException;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.BadRevisionFormatException;

public class AYamlerController extends AYamlerServiceGrpc.AYamlerServiceImplBase {

    private final Histogram getStrongModeBatchSize;
    private final Histogram getAbcServiceSlugBatchSize;
    private final AYamlerService aYamlerService;

    public AYamlerController(AYamlerService aYamlerService, CollectorRegistry collectorRegistry) {
        this.aYamlerService = aYamlerService;

        getStrongModeBatchSize = Histogram.build()
                .name(AYamlerApiMetrics.PREFIX + "get_strong_mode_batch_size")
                .help("Batch size of get_strong_mode_batch request")
                .buckets(new double[]{4, 16, 64, 256, 512, 2048})
                .register(collectorRegistry);

        getAbcServiceSlugBatchSize = Histogram.build()
                .name(AYamlerApiMetrics.PREFIX + "get_abc_service_slug_batch_size")
                .help("Batch size of get_abc_service_slug_batch request")
                .buckets(new double[]{4, 16, 64, 256, 512, 2048})
                .register(collectorRegistry);
    }

    @Override
    public void getStrongMode(GetStrongModeRequest request,
                              StreamObserver<GetStrongModeResponse> responseObserver) {
        Context.current().fork().run(() -> {
            var strongMode = getStrongMode(request);
            var response = GetStrongModeResponse.newBuilder()
                    .setStrongMode(strongMode)
                    .build();
            responseObserver.onNext(response);
            responseObserver.onCompleted();
        });
    }

    private Ayamler.StrongMode getStrongMode(GetStrongModeRequest request) {
        var revisionRequested = ArcRevision.of(request.getRevision());
        // TODO: check that login != "", when storage starts to send this field
        @Nullable
        var login = getLoginFromRequest(request);
        Optional<AYaml> aYamls;

        try {
            aYamls = aYamlerService.getStrongMode(
                    Path.of(request.getPath()), revisionRequested, login
            );
        } catch (BadRevisionFormatException e) {
            throw GrpcUtils.invalidArgumentException(e.getMessage());
        } catch (PathNotFoundException e) {
            return AYamlerProtoMappers.toProtoStrongMode(
                    request.getPath(), revisionRequested, Ayamler.StrongModeStatus.NOT_FOUND, login, false
            ).build();
        }
        return aYamls
                .map(aYaml -> AYamlerProtoMappers.toProtoStrongMode(
                        request.getPath(), revisionRequested, aYaml, login
                ))
                .orElseGet(() -> defaultProtoStrongMode(
                                request.getPath(), revisionRequested, login
                        )
                );
    }

    @Override
    public void getStrongModeBatch(
            GetStrongModeBatchRequest requestBatch,
            StreamObserver<GetStrongModeBatchResponse> responseBatchObserver
    ) {
        getStrongModeBatchSize.observe(requestBatch.getRequestCount());
        Context.current().fork().run(() -> {
            var response = GetStrongModeBatchResponse.newBuilder();
            var strongModes = requestBatch.getRequestList()
                    .stream()
                    .collect(Collectors.groupingBy(GetStrongModeRequest::getRevision))
                    .entrySet()
                    .stream()
                    .flatMap(revisionAndRequestList -> {
                        var revisionRequested = ArcRevision.of(revisionAndRequestList.getKey());
                        var requests = revisionAndRequestList.getValue();

                        // we need this map to keep original format of requested revision and path
                        var groupedRequests = requests.stream()
                                .collect(Collectors.toMap(
                                        request -> PathAndLogin.of(
                                                Path.of(request.getPath()),
                                                getLoginFromRequest(request)
                                        ),
                                        Function.identity()
                                ));

                        var strongModeMap = aYamlerService.getStrongMode(
                                revisionRequested, groupedRequests.keySet()
                        );

                        return groupedRequests.entrySet()
                                .stream()
                                .map(pathAndRequest -> {
                                    var path = pathAndRequest.getKey();
                                    var request = pathAndRequest.getValue();
                                    var login = getLoginFromRequest(request);

                                    return strongModeMap.get(path)
                                            .map(aYaml -> AYamlerProtoMappers.toProtoStrongMode(
                                                    request.getPath(), revisionRequested, aYaml, login
                                            ))
                                            .orElseGet(() -> defaultProtoStrongMode(
                                                    request.getPath(), revisionRequested, login
                                            ));
                                });
                    })
                    .collect(Collectors.toList());
            response.addAllStrongMode(strongModes);

            responseBatchObserver.onNext(response.build());
            responseBatchObserver.onCompleted();
        });
    }

    @Override
    public void getAbcServiceSlugBatch(GetAbcServiceSlugBatchRequest requestBatch,
                                       StreamObserver<GetAbcServiceSlugBatchResponse> responseBatchObserver) {
        getAbcServiceSlugBatchSize.observe(requestBatch.getRequestCount());
        Context.current().fork().run(() -> {
            var response = GetAbcServiceSlugBatchResponse.newBuilder();

            var abcServices = requestBatch.getRequestList()
                    .stream()
                    .collect(Collectors.groupingBy(GetAbcServiceSlugRequest::getRevision))
                    .entrySet()
                    .stream()
                    .flatMap(revisionAndRequestList -> {
                        var revisionRequested = ArcRevision.of(revisionAndRequestList.getKey());
                        var requests = revisionAndRequestList.getValue();

                        // we need this map to keep original format of requested revision and path
                        Map<Path, GetAbcServiceSlugRequest> groupedRequests = requests.stream()
                                .collect(Collectors.toMap(
                                        request -> Path.of(request.getPath()),
                                        Function.identity()
                                ));

                        Map<Path, Optional<AYaml>> abcSlugMap = aYamlerService.getAbcServiceSlugBatch(
                                revisionRequested, groupedRequests.keySet()
                        );

                        return groupedRequests.entrySet()
                                .stream()
                                .map(pathAndRequest -> {
                                    var aYaml = abcSlugMap.get(pathAndRequest.getKey()).orElse(null);
                                    return AYamlerProtoMappers.toProtoAbcService(
                                            /* We return path from request, cause path `pathAndRequest.getKey()`
                                                can be changed during normalization */
                                            pathAndRequest.getValue().getPath(),
                                            revisionRequested,
                                            aYaml
                                    );
                                });
                    })
                    .collect(Collectors.toList());
            response.addAllAbcService(abcServices);

            responseBatchObserver.onNext(response.build());
            responseBatchObserver.onCompleted();
        });
    }

    @Nullable
    private static String getLoginFromRequest(GetStrongModeRequest request) {
        return !request.getLogin().isEmpty() ? request.getLogin() : null;
    }

    private static Ayamler.StrongMode defaultProtoStrongMode(
            String path, ArcRevision revision, @Nullable String login
    ) {
        return AYamlerProtoMappers.toProtoStrongMode(
                        path, revision, Ayamler.StrongModeStatus.OFF, login, false
                )
                .build();
    }

}
