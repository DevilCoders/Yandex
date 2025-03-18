package ru.yandex.ci.storage.tests.api;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;

import static org.fest.assertions.Assertions.assertThat;

public class StoragePublicApiTests extends StorageTestsYdbTestBase {
    @Test
    public void streamsResults() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var build = exampleBuild(1L);
        var secondBuild = exampleBuild(2L);
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .results(build, secondBuild.toBuilder().setPath("ydb").setName("test").build())
                        .finish()
        );

        var check = storageTester.searchPublicApiTester().findLastCheckByPullRequest(
                StoragePublicApi.FindLastCheckByPullRequestRequest.newBuilder()
                        .setPullRequestId(42)
                        .build()
        ).getCheck();

        var circuits = storageTester.commonPublicApiTester().getCheckCircuits(
                StoragePublicApi.GetCheckCircuitsRequest.newBuilder()
                        .setCheckId(check.getId())
                        .build()
        ).getCircuitsList();

        assertThat(circuits).hasSize(1);

        var request = StoragePublicApi.StreamCheckResultsRequest.newBuilder()
                .setCheckId(check.getId())
                .setIterationType(circuits.get(0).getCircuitType())
                .setFilters(
                        StoragePublicApi.StreamCheckResultsRequest.Filters.newBuilder()
                                .build()
                )
                .build();

        var results = storageTester.rawDataPublicApiTester().streamCheckIterationResults(request);
        assertThat(results).hasSize(4);

        results = storageTester.rawDataPublicApiTester().streamCheckIterationResults(
                request.toBuilder().setFilters(
                        StoragePublicApi.StreamCheckResultsRequest.Filters.newBuilder()
                                .setPathExpression("y.*")
                                .build()
                ).build()
        );
        assertThat(results).hasSize(2);
    }
}
