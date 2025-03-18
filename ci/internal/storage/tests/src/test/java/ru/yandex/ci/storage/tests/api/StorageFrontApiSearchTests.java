package ru.yandex.ci.storage.tests.api;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;

import static org.assertj.core.api.Assertions.assertThat;

public class StorageFrontApiSearchTests extends StorageTestsYdbTestBase {
    @Test
    public void singleResult() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        storageTester.writeAndDeliver(writer -> writer.to(registration.getTasks()).results(exampleBuild(1L)).finish());

        var textSearch = db.currentOrReadOnly(() -> db.checkTextSearch().findAll());
        assertThat(textSearch).hasSize(2);

        var iterationId = registration.getIteration().getId();

        var results = storageTester.frontApi().getSuggest(
                iterationId, Common.CheckSearchEntityType.CSET_TEST_NAME, "ja*"
        );
        assertThat(results).hasSize(1);
        assertThat(results.get(0)).isEqualTo("java");

        var suiteSearch = storageTester.frontApi().searchSuites(
                iterationId,
                StorageFrontApi.SuiteSearch.newBuilder()
                        .setTestName("java")
                        .setToolchain("@all")
                        .addResultType(Common.ResultType.RT_BUILD)
                        .setCategory(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .setStatusFilter(StorageFrontApi.StatusFilter.STATUS_PASSED)
                        .build()
        );

        assertThat(suiteSearch.getSuitesList()).hasSize(1);
    }
}
