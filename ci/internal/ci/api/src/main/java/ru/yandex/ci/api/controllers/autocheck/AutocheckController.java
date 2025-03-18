package ru.yandex.ci.api.controllers.autocheck;

import io.grpc.stub.StreamObserver;

import ru.yandex.ci.autocheck.api.AutocheckApi;
import ru.yandex.ci.autocheck.api.AutocheckServiceGrpc;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.proto.ProtoMappers;

public class AutocheckController extends AutocheckServiceGrpc.AutocheckServiceImplBase {

    private final AutocheckService autocheckService;

    public AutocheckController(AutocheckService autocheckService) {
        this.autocheckService = autocheckService;
    }

    @Override
    public void getFastTargets(AutocheckApi.GetFastTargetsRequest request,
                               StreamObserver<AutocheckApi.GetFastTargetsResponse> responseObserver) {
        AutocheckLaunchConfig autocheckInfo = autocheckService.findTrunkAutocheckLaunchParams(
                ProtoMappers.toArcRevision(request.getPrevRevision()),
                ProtoMappers.toArcRevision(request.getRevision()),
                null
        );

        var response = AutocheckApi.GetFastTargetsResponse.newBuilder()
                .setFast(!autocheckInfo.getFastTargets().isEmpty())
                .setMode(toModeOld(autocheckInfo))
                .addAllTargets(autocheckInfo.getFastTargets())
                .addAllInvalidConfigs(autocheckInfo.getInvalidAYamls())
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void getAutocheckInfo(AutocheckApi.GetAutocheckInfoRequest request,
                                 StreamObserver<AutocheckApi.GetAutocheckInfoResponse> responseObserver) {
        if (request.getCheckAuthor().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("check_author is empty");
        }

        AutocheckLaunchConfig autocheckInfo = autocheckService.findTrunkAutocheckLaunchParams(
                ProtoMappers.toArcRevision(request.getPrevRevision()),
                ProtoMappers.toArcRevision(request.getRevision()),
                request.getCheckAuthor()
        );

        var response = AutocheckApi.GetAutocheckInfoResponse.newBuilder()
                .setFastMode(toMode(autocheckInfo))
                .addAllFastTargets(autocheckInfo.getFastTargets())
                .addAllInvalidConfigs(autocheckInfo.getInvalidAYamls())
                .setPoolName(autocheckInfo.getPoolName())
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Deprecated
    private AutocheckApi.GetFastTargetsResponse.Mode toModeOld(AutocheckLaunchConfig autocheckInfo) {
        if (autocheckInfo.getFastTargets().isEmpty()) {
            return AutocheckApi.GetFastTargetsResponse.Mode.DISABLED;
        }
        if (autocheckInfo.isSequentialMode()) {
            return AutocheckApi.GetFastTargetsResponse.Mode.SEQUENTIAL;
        }
        return AutocheckApi.GetFastTargetsResponse.Mode.PARALLEL;
    }

    private AutocheckApi.GetAutocheckInfoResponse.FastMode toMode(AutocheckLaunchConfig autocheckInfo) {
        if (autocheckInfo.getFastTargets().isEmpty()) {
            return AutocheckApi.GetAutocheckInfoResponse.FastMode.DISABLED;
        }
        if (autocheckInfo.isSequentialMode()) {
            return AutocheckApi.GetAutocheckInfoResponse.FastMode.SEQUENTIAL;
        }
        return AutocheckApi.GetAutocheckInfoResponse.FastMode.PARALLEL;
    }
}
