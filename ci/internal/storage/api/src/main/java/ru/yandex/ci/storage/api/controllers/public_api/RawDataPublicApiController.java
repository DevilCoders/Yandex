package ru.yandex.ci.storage.api.controllers.public_api;

import java.util.function.Predicate;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.logging.log4j.util.Strings;

import yandex.cloud.repository.db.IsolationLevel;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.storage.api.RawDataPublicApiGrpc;
import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.proto.PublicProtoMappers;

@Slf4j
@RequiredArgsConstructor
public class RawDataPublicApiController extends RawDataPublicApiGrpc.RawDataPublicApiImplBase {
    private final CiStorageDb db;

    @Override
    public void streamCheckIterationResults(
            StoragePublicApi.StreamCheckResultsRequest request,
            StreamObserver<StoragePublicApi.TestResultPublicViewModel> responseObserver
    ) {
        this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY).run(
                () -> streamCheckIterationResultsInTx(request, responseObserver)
        );
    }

    private void streamCheckIterationResultsInTx(
            StoragePublicApi.StreamCheckResultsRequest request,
            StreamObserver<StoragePublicApi.TestResultPublicViewModel> responseObserver
    ) {
        log.info("Stream check results request: {}", request);

        var checkId = CheckEntity.Id.of(request.getCheckId());

        var resultTypes = request.getFilters().getResultTypesList();
        var resultTypePredicate = resultTypes.size() > 0 ?
                (Predicate<Common.ResultType>) resultTypes::contains :
                (Predicate<Common.ResultType>) ignore -> true;
        var pathPredicate = getTextFilterPredicate(request.getFilters().getPathExpression());
        var namePredicate = getTextFilterPredicate(request.getFilters().getTestNameExpression());
        var subtestNamePredicate = getTextFilterPredicate(request.getFilters().getSubtestNameExpression());


        db.testResults().streamCircuit(checkId, request.getIterationType())
                .filter(
                        result ->
                                resultTypePredicate.test(result.getResultType()) &&
                                        pathPredicate.test(result.getPath()) &&
                                        namePredicate.test(result.getName()) &&
                                        subtestNamePredicate.test(result.getSubtestName())
                )
                .map(PublicProtoMappers::toProtoResult)
                .forEach(responseObserver::onNext);

        responseObserver.onCompleted();
    }

    private Predicate<String> getTextFilterPredicate(String value) {
        try {
            return Strings.isNotEmpty(value) ? Pattern.compile(value).asMatchPredicate() : ignore -> true;
        } catch (PatternSyntaxException e) {
            throw GrpcUtils.invalidArgumentException(
                    "Invalid pattern: %s, message: %s".formatted(value, e.getMessage())
            );
        }
    }
}
