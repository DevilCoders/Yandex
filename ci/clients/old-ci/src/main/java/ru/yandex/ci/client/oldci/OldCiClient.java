package ru.yandex.ci.client.oldci;

import java.util.List;

import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.PATCH;
import retrofit2.http.Path;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class OldCiClient {
    private final CiApi ciApi;

    private OldCiClient(HttpClientProperties httpClientProperties) {
        this.ciApi = RetrofitClient.builder(httpClientProperties, getClass())
                .build(CiApi.class);
    }

    public static OldCiClient create(HttpClientProperties httpClientProperties) {
        return new OldCiClient(httpClientProperties);
    }

    //

    public CiCheck getCheck(String id) {
        return ciApi.check(id);
    }

    public void recheckTargets(String projectName, List<CheckRecheckingTargets> targets) {
        ciApi.recheckTargets(projectName, new RecheckTargetsRequest(targets));
    }

    public void muteTest(String testId, String toolchain, String comment) {
        doMuteUnmute(testId, toolchain, false, comment);
    }

    public void unmuteTest(String testId, String toolchain, String comment) {
        doMuteUnmute(testId, toolchain, true, comment);
    }

    public void cancelCheck(String checkId) {
        ciApi.cancelCheck(checkId, new CancelCheckRequest());
    }

    private void doMuteUnmute(String testId, String toolchain, boolean notificationsEnabled, String comment) {
        ciApi.muteUnmuteTest(testId, toolchain, new MuteUnmuteRequest(notificationsEnabled, comment));
    }

    interface CiApi {
        @GET("/api/v1.0/checks/{checkId}")
        CiCheck check(@Path("checkId") String checkId);

        @PATCH("/api/v1.0/checks/{checkId}")
        Response<Void> recheckTargets(@Path("checkId") String checkId,
                                      @Body RecheckTargetsRequest request);

        @PATCH("/api/v1.0/checks/{checkId}")
        Response<Void> cancelCheck(@Path("checkId") String checkId,
                                   @Body CancelCheckRequest request);

        @PATCH("/api/v1.0/tests/{testId}")
        Response<Void> muteUnmuteTest(@Path("testId") String testId,
                                      @Query("toolchain") String toolchain,
                                      @Body MuteUnmuteRequest request);
    }
}
