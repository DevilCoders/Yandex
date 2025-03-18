package ru.yandex.ci.client.testenv;

import java.util.List;
import java.util.Objects;
import java.util.Optional;

import com.google.common.base.Preconditions;
import retrofit2.http.Body;
import retrofit2.http.DELETE;
import retrofit2.http.GET;
import retrofit2.http.PATCH;
import retrofit2.http.POST;
import retrofit2.http.Path;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.testenv.model.PatchProjectRequestDto;
import ru.yandex.ci.client.testenv.model.RecheckRequest;
import ru.yandex.ci.client.testenv.model.StartCheckV2RequestDto;
import ru.yandex.ci.client.testenv.model.StartCheckV2ResponseDto;
import ru.yandex.ci.client.testenv.model.TestResultDto;
import ru.yandex.ci.client.testenv.model.TestResultResponseDto;
import ru.yandex.ci.client.testenv.model.TestenvJob;
import ru.yandex.ci.client.testenv.model.TestenvListProject;
import ru.yandex.ci.client.testenv.model.TestenvProject;

public class TestenvClient {
    private final TestEnvApi api;

    private TestenvClient(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .snakeCaseNaming()
                .build(TestEnvApi.class);
    }

    public static TestenvClient create(HttpClientProperties httpClientProperties) {
        return new TestenvClient(httpClientProperties);
    }

    public String recheck(RecheckRequest request) {
        return api.recheck(request);
    }

    public List<TestenvListProject> getProjects() {
        return api.getProjects();
    }

    public TestenvProject getProject(String projectName) {
        return api.getProject(projectName);
    }

    public List<TestenvJob> getJobs(String projectName) {
        return api.getJobs(projectName);
    }

    public StartCheckV2ResponseDto createPrecommitCheckV2(StartCheckV2RequestDto request) {
        return api.createPrecommitCheckV2(request);
    }


    public void stopProject(String projectName, String comment) {
        var body = new PatchProjectRequestDto("stopped", comment);
        api.patchProject(projectName, body);
    }

    public void dropProject(String projectName, Confirmation confirmation) {
        Preconditions.checkState(confirmation == Confirmation.I_UNDERSTAND_THAT_THIS_METHOD_IS_DANGEROUS);
        api.dropProject(projectName);
    }

    public Optional<TestResultDto> getPostcommitTestResult(String testName, int revision) {
        var response = api.getTestResults("autocheck-trunk", testName, revision, false, false, 1);
        return Objects.requireNonNullElse(response.getRows(), List.<TestResultDto>of())
                .stream()
                .findFirst();
    }


    public enum Confirmation {
        I_UNDERSTAND_THAT_THIS_METHOD_IS_DANGEROUS;
    }

    interface TestEnvApi {

        @POST("/api/te/v1.0/autocheck/recheck")
        String recheck(@Body RecheckRequest request);

        @GET("/api/te/v1.0/projects/?type=custom_check")
        List<TestenvListProject> getProjects();

        @GET("/api/te/v1.0/projects/{projectName}")
        TestenvProject getProject(@Path("projectName") String projectName);

        @PATCH("/api/te/v1.0/projects/{projectName}")
        TestenvProject patchProject(@Path("projectName") String projectName, @Body PatchProjectRequestDto request);

        @DELETE("/api/te/v1.0/projects/{projectName}")
        TestenvProject dropProject(@Path("projectName") String projectName);

        @GET("/api/te/v1.0/projects/{projectName}/jobs")
        List<TestenvJob> getJobs(@Path("projectName") String projectName);

        @POST("/api/te/v1.0/autocheck/precommit/checks_v2")
        StartCheckV2ResponseDto createPrecommitCheckV2(@Body StartCheckV2RequestDto request);

        @GET("/handlers/grids/testResults")
        TestResultResponseDto getTestResults(@Query("database") String database,
                                             @Query("test_name") String testName,
                                             @Query("revision") int revision,
                                             @Query("hide_filtered") boolean hideFiltered,
                                             @Query("hide_not_checked") boolean hideNotChecked,
                                             @Query("limit") int limit);

    }
}

