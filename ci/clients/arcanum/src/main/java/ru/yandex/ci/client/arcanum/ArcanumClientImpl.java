package ru.yandex.ci.client.arcanum;

import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;
import javax.annotation.ParametersAreNonnullByDefault;

import com.fasterxml.jackson.databind.JsonNode;
import com.google.common.base.Preconditions;
import lombok.Value;
import org.apache.http.HttpStatus;
import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.PATCH;
import retrofit2.http.POST;
import retrofit2.http.PUT;
import retrofit2.http.Path;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.base.http.RetrofitClient;

@ParametersAreNonnullByDefault
public class ArcanumClientImpl implements ArcanumClient {
    private final ArcanumApiRead apiRead;
    private final ArcanumApiModify apiModify;

    private ArcanumClientImpl(
            HttpClientProperties properties,
            boolean readOnly
    ) {
        var builder = RetrofitClient.builder(properties, getClass())
                .snakeCaseNaming();

        this.apiRead = builder.build(ArcanumApiRead.class);
        this.apiModify = readOnly
                ? ArcanumApiModifyDenied.INSTANCE
                : builder.build(ArcanumApiModify.class);
    }

    public static ArcanumClientImpl create(HttpClientProperties properties, boolean readOnly) {
        return new ArcanumClientImpl(properties, readOnly);
    }

    @Override
    public void setMergeRequirementStatus(
            long reviewRequestId,
            long diffSetId,
            UpdateCheckStatusRequest updateRequest
    ) {
        apiModify.setMergeRequirementStatus(reviewRequestId, diffSetId, updateRequest);
    }

    public String restartCheck(String id) {
        return apiModify.restartCheck(id);
    }

    public ArcanumReviewActivityDto getReviewRequestActivities(long reviewRequestId) {
        return apiRead.getReviewRequestActivities(reviewRequestId);
    }

    @Override
    public Optional<ArcanumReviewDataDto> getReviewRequest(long reviewRequestId) {
        return getReviewRequestData(
                reviewRequestId, "id,summary,vcs(type,from_branch,to_branch),active_diff_set(id),approvers"
        );
    }

    @Override
    public GroupsDto getGroups() {
        return apiRead.getGroups();
    }

    @Override
    public Optional<ArcanumReviewDataDto> getReviewSummaryAndDescription(long reviewRequestId) {
        return getReviewRequestData(
                reviewRequestId, "id,summary,description"
        );
    }

    public Optional<ArcanumReviewDataDto> getReviewRequestBySvnRevision(long svnRevision) {
        // TODO собирать данные в ревью в релизные ветки
        try {
            var dataList = apiRead.getReviewRequestBySvnRevision(
                    svnRevision,
                    "id,vcs(type,from_branch,to_branch),active_diff_set(id),approvers"
            );
            if (dataList.getData().isEmpty()) {
                return Optional.empty();
            }
            Preconditions.checkState(
                    dataList.getData().size() == 1,
                    "found more that one review on commit %s: %s",
                    svnRevision,
                    dataList.getData()
            );
            return Optional.of(dataList.getData().get(0));
        } catch (HttpException ex) {
            if (ex.getHttpCode() == HttpStatus.SC_NOT_FOUND) {
                return Optional.empty();
            }
            throw new RuntimeException(ex);
        }
    }

    @Override
    public void setMergeRequirement(
            long reviewRequestId,
            ArcanumMergeRequirementId requirementId,
            Require require) {

        var check = new ArcanumMergeRequirementDto(requirementId, require.isRequired(), require.getDisableReason());
        apiModify.setMergeRequirement(reviewRequestId, check);
    }

    public void setMergeRequirementStatus(
            long reviewRequestId,
            long diffSetId,
            ArcanumMergeRequirementId requirementId,
            ArcanumMergeRequirementDto.Status status
    ) {
        setMergeRequirementStatus(reviewRequestId, diffSetId, requirementId, status, null);
    }

    public void setMergeRequirementStatus(
            long reviewRequestId,
            long diffSetId,
            ArcanumMergeRequirementId requirementId,
            ArcanumMergeRequirementDto.Status status,
            @Nullable String checkUri
    ) {
        setMergeRequirementStatus(reviewRequestId, diffSetId, requirementId, status, checkUri, null);
    }

    public void setMergeRequirementStatus(
            long reviewRequestId,
            long diffSetId,
            ArcanumMergeRequirementId requirementId,
            ArcanumMergeRequirementDto.Status status,
            @Nullable String checkUri,
            @Nullable String description
    ) {
        var check = UpdateCheckStatusRequest.builder()
                .requirementId(requirementId)
                .status(status)
                .description(description)
                .systemCheckUri(checkUri)
                .build();
        setMergeRequirementStatus(reviewRequestId, diffSetId, check);
    }


    public void createReviewRequestComment(long reviewRequestId, String comment) {
        apiModify.createReviewRequestComment(reviewRequestId, new ReviewRequestComment(comment));
    }

    public List<ArcanumMergeRequirementDto> getMergeRequirements(long reviewRequestId) {
        return getReviewRequestData(reviewRequestId,
                "id,checks(system,type,required,satisfied,status,disabling_policy)")
                .map(ArcanumReviewDataDto::getChecks)
                .orElse(List.of());
    }

    public Optional<ArcanumReviewDataDto.DiffSet> getActiveDiffSet(long reviewRequestId) {
        Optional<ArcanumReviewDataDto> arcanumReviewData = getReviewRequestData(
                reviewRequestId,
                "active_diff_set(id,gsid,description,status,patch_url,arc_branch_heads(from_id,to_id,merge_id))"
        );
        return arcanumReviewData.map(ArcanumReviewDataDto::getActiveDiffSet);
    }

    public List<ArcanumReviewDataDto.DiffSet> getAllDiffSets(long reviewRequestId) {
        Optional<ArcanumReviewDataDto> arcanumReviewData = getReviewRequestData(
                reviewRequestId,
                "diff_sets(id,gsid,description,status,patch_url," +
                        "arc_branch_heads(from_id,to_id,merge_id))"
        );
        return arcanumReviewData.isPresent() ? arcanumReviewData.get().getDiffSets() : List.of();
    }

    public String generateUrlForArcPathAtTrunkHead(java.nio.file.Path path) {
        return java.nio.file.Path.of("/arc_vcs").resolve(path).toString();
    }

    public Optional<ArcanumReviewDataDto> getReviewRequestData(long reviewRequestId, String fields) {
        var arcanumReviewRequest = apiRead.getReviewRequestData(reviewRequestId, fields);
        return arcanumReviewRequest.getData() == null ? Optional.empty() : Optional.of(arcanumReviewRequest.getData());
    }

    /**
     * @throws HttpException with code 400, when diffSetId != activeDiffSetId of pull request
     */
    @Override
    public void setMergeRequirementsWithCheckingActiveDiffSet(
            long reviewRequestId,
            long diffSetId,
            List<UpdateCheckRequirementRequestDto> requirements) {
        apiModify.setMergeRequirementsWithCheckingActiveDiffSet(reviewRequestId, diffSetId, requirements);
    }

    public void registerCiCheck(long diffSetId, RegisterCheckRequestDto registerCheckRequest) {
        apiModify.registerCiCheck(diffSetId, registerCheckRequest);
    }

    @Value
    public static class ArcanumReviewRequestSingleData {
        ArcanumReviewDataDto data;
    }

    @Value
    static class ArcanumReviewRequestMultipleData {
        List<ArcanumReviewDataDto> data;
    }

    @Value
    static class ArcanumReviewCiData {
        String system;
        String type;
        String systemCheckId;
    }

    @Value
    static class ArcanumReviewRequestMultipleCiData {
        List<ArcanumReviewCiData> data;
    }

    @Value
    public static class ReviewRequestComment {
        String content;
    }

    // https://a.yandex-team.ru/api/swagger/index.html#/
    interface ArcanumApiRead {
        @GET("/api/v2/groups?fields=name,members(name)")
        GroupsDto getGroups();

        @GET("/api/review/review-request/{reviewRequestId}/activity")
        ArcanumReviewActivityDto getReviewRequestActivities(@Path("reviewRequestId") long reviewRequestId);

        @GET("/api/v1/merge-commits/{svnRevision}/review-requests")
        ArcanumReviewRequestMultipleData getReviewRequestBySvnRevision(
                @Path("svnRevision") long svnRevision,
                @Query(value = "fields", encoded = true) String fields);

        @GET("/api/v1/review-requests/{reviewRequestId}")
        ArcanumReviewRequestSingleData getReviewRequestData(
                @Path("reviewRequestId") long reviewRequestId,
                @Query(value = "fields", encoded = true) String fields);

        @GET("/api/v2/users/{login}/settings")
        JsonNode getUserSettings(@Path("login") String login);
    }

    interface ArcanumApiModify {

        @POST("/api/dev/ci-checks/{id}/force-restart")
        String restartCheck(@Path("id") String id);

        @POST("/api/v1/review-requests/{reviewRequestId}/checks")
        Response<Void> setMergeRequirement(
                @Path("reviewRequestId") long reviewRequestId,
                @Body ArcanumMergeRequirementDto request);

        @POST("/api/v1/review-requests/{reviewRequestId}/diff-sets/{diffSetId}/checks")
        Response<Void> setMergeRequirementStatus(
                @Path("reviewRequestId") long reviewRequestId,
                @Path("diffSetId") long diffSetId,
                @Body UpdateCheckStatusRequest request
        );

        @POST("/api/v1/review-requests/{reviewRequestId}/comments")
        Response<Void> createReviewRequestComment(
                @Path("reviewRequestId") long reviewRequestId,
                @Body ReviewRequestComment comment);

        @PATCH("/api/v1/review-requests/{reviewRequestId}/check-requirements")
        Response<Void> setMergeRequirementsWithCheckingActiveDiffSet(
                @Path("reviewRequestId") long reviewRequestId,
                @Query("diff_set_id") long diffSetId,
                @Body List<UpdateCheckRequirementRequestDto> requirements);

        @PUT("/api/v1/diff-sets/{diff_set_id}/ci-check")
        Response<Void> registerCiCheck(
                @Path("diff_set_id") long diffSetId,
                @Body RegisterCheckRequestDto registerCheckRequest);
    }
}
