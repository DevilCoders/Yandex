package ru.yandex.ci.client.tsum;

import java.util.List;

import retrofit2.http.GET;
import retrofit2.http.Path;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class TsumClient {

    private final TsumApi api;

    private TsumClient(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(TsumApi.class);
    }

    public static TsumClient create(HttpClientProperties httpClientProperties) {
        return new TsumClient(httpClientProperties);
    }

    public List<TsumListProject> getProjects() {
        return api.getProjects();
    }

    public TsumProject getProject(String projectId) {
        return api.getProject(projectId);
    }

    public TsumPipelineConfiguration getPipelineConfiguration(String pipelineId, int version) {
        return api.getPipelineConfiguration(pipelineId, version);
    }

    public TsumPipelineFullConfiguration getPipelineFullConfiguration(String configurationId) {
        return api.getPipelineFullConfiguration(configurationId);
    }

    interface TsumApi {
        @GET("/api/projects")
        List<TsumListProject> getProjects();

        @GET("/api/projects/{projectId}")
        TsumProject getProject(
                @Path("projectId") String projectId);

        @GET("/api/pipelines/{pipelineId}/configurations/{version}")
        TsumPipelineConfiguration getPipelineConfiguration(
                @Path("pipelineId") String pipelineId,
                @Path("version") int version);

        @GET("/api/pipelines/*/configurations/{configurationId}/full")
        TsumPipelineFullConfiguration getPipelineFullConfiguration(
                @Path("configurationId") String configurationId);
    }

}
