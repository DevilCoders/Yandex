package ru.yandex.ci.storage.tests.api;

import java.util.Comparator;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.exceptions.CheckIsReadonlyException;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;
import ru.yandex.ci.storage.tests.tester.StorageTesterRegistrar;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.AssertionsForClassTypes.assertThatThrownBy;

public class StorageFrontApiTests extends StorageTestsYdbTestBase {
    @Test
    public void runCommandTests() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        storageTester.writeAndDeliver(
                writer -> writer.to(registration.getTasks())
                        .results(
                                exampleSuite(100),
                                exampleChunk(10, 100, Common.ResultType.RT_TEST_MEDIUM),
                                exampleTest(1, 10, 100, Common.ResultType.RT_TEST_MEDIUM)
                        )
                        .finish()
        );

        var search = StorageFrontApi.SuiteSearch.newBuilder()
                .addResultType(Common.ResultType.RT_TEST_MEDIUM)
                .setCategory(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                .setStatusFilter(StorageFrontApi.StatusFilter.STATUS_PASSED)
                .setToolchain(TestEntity.ALL_TOOLCHAINS)
                .build();

        var suiteSearch = storageTester.frontApi().searchSuites(registration.getIteration().getId(), search);

        assertThat(suiteSearch.getSuitesList()).hasSize(1);

        var suite = suiteSearch.getSuitesList().get(0);
        assertThat(storageTester.frontApi().getRunCommand(suite.getId()))
                .isEqualTo(
                        "ya make -DAUTOCHECK=yes -A --ignore-recurses --test-size medium --test-type example_suite ci"
                );

        var diffs = storageTester.frontApi().listSuite(search, suite.getId()).stream()
                .sorted(Comparator.comparing(StorageFrontApi.DiffViewModel::getName))
                .collect(Collectors.toList());
        assertThat(diffs).hasSize(2);

        assertThat(storageTester.frontApi().getRunCommand(diffs.get(0).getId())).isEqualTo(
                "ya make -DAUTOCHECK=yes -A --ignore-recurses " +
                        "--test-size medium --test-type example_suite -F 'example_chunk' ci"
        );

        var command = "ya make -DAUTOCHECK=yes -A --ignore-recurses " +
                "--test-size medium --test-type example_suite -F 'example_chunk' -F " +
                "'test::sub' ci";
        assertThat(storageTester.frontApi().getRunCommand(diffs.get(1).getId())).isEqualTo(command);
        var testId = CheckProtoMappers.toDiffId(diffs.get(1).getId()).getCombinedTestId();
        var statusId = CheckProtoMappers.toProtoTestStatusId(new TestStatusEntity.Id(Trunk.name(), testId));
        assertThat(storageTester.frontApi().getRunCommand(statusId)).isEqualTo(command);
    }

    @Test
    public void cancelsCheck() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());
        storageTester.frontApi().cancelCheck(registration.getCheck().getId());
        var check = storageTester.getCheck(registration.getCheck().getId());
        assertThat(check.getStatus()).isEqualTo(Common.CheckStatus.CANCELLING);
        assertThat(check.getAttributeOrDefault(Common.StorageAttribute.SA_CANCELLED_BY, "")).isEqualTo("user42");
        storageTester.executeAllOnetimeTasks();
        assertThat(testCiClient.getCancelRequests()).hasSize(1);

        logbrokerService.deliverAllMessages();
        check = storageTester.getCheck(registration.getCheck().getId());
        assertThat(check.getStatus()).isEqualTo(Common.CheckStatus.CANCELLED);

        assertThatThrownBy(
                () -> storageTester.register(StorageTesterRegistrar::fullIteration)
        ).isInstanceOf(CheckIsReadonlyException.class);
    }
}
