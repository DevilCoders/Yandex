package ru.yandex.ci.storage.tests.api;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.api.StorageFrontTestsApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;

import static org.fest.assertions.Assertions.assertThat;

public class StorageFrontTestsApiTests extends StorageTestsYdbTestBase {
    @Test
    public void searchAllProjects() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        var build = exampleBuild(1L);
        var secondBuild = exampleBuild(2L);
        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .results(
                                build.toBuilder().setId(build.getId().toBuilder().setToolchain("A").build()).build(),
                                build.toBuilder().setId(build.getId().toBuilder().setToolchain("B").build()).build(),
                                secondBuild.toBuilder().setPath("ydb").setName("test").build()
                        )
                        .finish()
        );

        // all projects
        var request = StorageFrontTestsApi.SearchTestsRequest.newBuilder()
                .setBranch(Trunk.name())
                .addResultTypes(Common.ResultType.RT_BUILD)
                .build();

        var tests = storageTester.testsApi().getTestInfo(request).getTestsList();

        assertThat(tests).hasSize(2);

        // by project
        request = StorageFrontTestsApi.SearchTestsRequest.newBuilder()
                .setBranch(Trunk.name())
                .setProject("ci")
                .addResultTypes(Common.ResultType.RT_BUILD)
                .build();

        tests = storageTester.testsApi().getTestInfo(request).getTestsList();

        assertThat(tests).hasSize(2);
        assertThat(tests.get(0).getId().getHid()).isEqualTo("1");
        assertThat(tests.get(1).getId().getHid()).isEqualTo("2");
    }

    @Test
    public void paging() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        storageTester.writeAndDeliver(
                registration,
                writer -> writer.toAll()
                        .results(
                                exampleBuild(1L).toBuilder().setPath("a").build(),
                                exampleBuild(2L).toBuilder().setPath("b").build(),
                                exampleBuild(3L).toBuilder().setPath("c").build(),
                                exampleBuild(4L).toBuilder().setPath("d").build(),
                                exampleBuild(5L).toBuilder().setPath("e").build()
                        )
                        .finish()
        );

        // all projects
        var request = StorageFrontTestsApi.SearchTestsRequest.newBuilder()
                .setBranch(Trunk.name())
                .setPageSize(2)
                .build();

        var response = storageTester.testsApi().getTestInfo(request);

        checkFirstPage(response);

        response = storageTester.testsApi().getTestInfo(request.toBuilder().setNext(response.getNext()).build());

        checkSecondPage(response);

        response = storageTester.testsApi().getTestInfo(request.toBuilder().setNext(response.getNext()).build());

        assertThat(response.getTestsList()).hasSize(1);
        assertThat(response.getTestsList().get(0).getToolchainsList().get(0).getPath()).isEqualTo("e");
        assertThat(response.getPrevious().getPath()).isEqualTo("e");
        assertThat(response.getNext().getPath()).isEmpty();

        response = storageTester.testsApi().getTestInfo(
                request.toBuilder().setPrevious(response.getPrevious()).build()
        );

        checkSecondPage(response);

        response = storageTester.testsApi().getTestInfo(
                request.toBuilder().setPrevious(response.getPrevious()).build()
        );

        checkFirstPage(response);
    }

    private void checkFirstPage(StorageFrontTestsApi.SearchTestsResponse response) {
        assertThat(response.getTestsList()).hasSize(2);
        assertThat(response.getTestsList().get(0).getToolchainsList().get(0).getPath()).isEqualTo("a");
        assertThat(response.getTestsList().get(1).getToolchainsList().get(0).getPath()).isEqualTo("b");
        assertThat(response.getPrevious().getPath()).isEmpty();
        assertThat(response.getNext().getPath()).isEqualTo("c");
    }

    private void checkSecondPage(StorageFrontTestsApi.SearchTestsResponse response) {
        assertThat(response.getTestsList()).hasSize(2);
        assertThat(response.getTestsList().get(0).getToolchainsList().get(0).getPath()).isEqualTo("c");
        assertThat(response.getTestsList().get(1).getToolchainsList().get(0).getPath()).isEqualTo("d");
        assertThat(response.getPrevious().getPath()).isEqualTo("c");
        assertThat(response.getNext().getPath()).isEqualTo("e");
    }
}
