package ru.yandex.ci.client.teamcity;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Preconditions;
import lombok.Value;
import retrofit2.http.GET;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class TeamcityClient {

    private static final int LIMIT = 10_000;

    private final TeamCityApi api;

    private TeamcityClient(HttpClientProperties httpClientProperties) {
        api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(TeamCityApi.class);
    }

    public static TeamcityClient create(HttpClientProperties httpClientProperties) {
        return new TeamcityClient(httpClientProperties);
    }

    private void checkLimit(int count) {
        Preconditions.checkState(
                count < LIMIT,
                "Too many elements (>=%s). Client does not support paging yet",
                LIMIT
        );
    }

    public List<TeamcityVcsRoot> getVcsRoots(String locatorQuery) {
        var response = api.getVcsRoots(
                locatorQuery + ",count:" + LIMIT,
                "vcs-root(id,href,project())"
        );
        checkLimit(response.getVcsRoots().size());
        return response.getVcsRoots();
    }

    public List<TeamcityVcsRoot> getArcRoots() {
        return getVcsRoots("type:arc");
    }

    public List<TeamcityVcsRoot> getArcadiaSvnRoots() {
        return getVcsRoots("property:(name:url,value:arcadia.yandex.ru,matchType:contains)");
    }

    public List<TeamcityBuildType> getBuildTypes(String locatorQuery) {
        var response = api.getBuildType(locatorQuery + ",count:" + LIMIT);
        checkLimit(response.getBuildTypes().size());
        return response.getBuildTypes();
    }

    public List<TeamcityBuildType> getBuildTypesWithArcRoots() {
        return getBuildTypes("vcsRoot:(type:arc),paused:false");
    }

    public List<TeamcityBuildType> getBuildTypesWithArcadiaSvnRoots() {
        return getBuildTypes("vcsRoot:(property:(name:url,value:arcadia.yandex.ru,matchType:contains)),paused:false");
    }

    @Value
    private static class VscRootsResponse {
        @JsonProperty("vcs-root")
        List<TeamcityVcsRoot> vcsRoots;
    }

    @Value
    private static class BuildTypesResponse {
        @JsonProperty("buildType")
        List<TeamcityBuildType> buildTypes;
    }


    interface TeamCityApi {
        @GET("/app/rest/vcs-roots")
        VscRootsResponse getVcsRoots(
                @Query(value = "locator", encoded = true) String locator,
                @Query(value = "fields", encoded = true) String fields);

        @GET("/app/rest/buildTypes")
        BuildTypesResponse getBuildType(
                @Query(value = "locator", encoded = true) String locator);

    }
}
