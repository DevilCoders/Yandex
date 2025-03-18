package ru.yandex.ci.client.arcanum;

import java.io.Closeable;
import java.io.UncheckedIOException;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import io.netty.handler.codec.http.HttpMethod;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.JsonBody;
import org.mockserver.model.RegexBody;
import org.mockserver.verify.VerificationTimes;

import ru.yandex.ci.client.arcanum.util.BodyVerificationMode;
import ru.yandex.ci.client.arcanum.util.RevisionNumberPullRequestIdPair;
import ru.yandex.ci.util.ResourceUtils;

import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.verify.VerificationTimes.atLeast;

public class ArcanumTestServer implements Closeable {

    private static final ObjectMapper MAPPER = new ObjectMapper()
            .setPropertyNamingStrategy(PropertyNamingStrategies.SNAKE_CASE)
            .setSerializationInclusion(JsonInclude.Include.NON_NULL);

    private final ClientAndServer server = new ClientAndServer();

    @Override
    public void close() {
        server.close();
    }

    public void mockGetReviewRequestData(long pullRequestId, String responseTextResource) {
        var response = ResourceUtils.textResource(responseTextResource);
        server.when(request("/api/v1/review-requests/" + pullRequestId)
                .withMethod("GET")
                .withQueryStringParameter("fields", ".*")
        ).respond(response().withBody(JsonBody.json(response)));
    }

    public void mockSetMergeRequirement() {
        server.when(request("/api/v1/review-requests/\\d+/checks").withMethod("POST"))
                .respond(response().withStatusCode(200));
    }

    public void mockSetMergeRequirementStatus() {
        server.when(request("/api/v1/review-requests/\\d+/diff-sets/\\d+/checks").withMethod("POST"))
                .respond(response().withStatusCode(200));
    }

    public void mockCreateReviewRequestComment() {
        server.when(request("/api/v1/review-requests/\\d+/comments").withMethod("POST"))
                .respond(response().withStatusCode(200));
    }

    public void mockGetReviewRequestBySvnRevision(RevisionNumberPullRequestIdPair... dataList) {
        for (var data : dataList) {
            server.when(request("/api/v1/merge-commits/%d/review-requests".formatted(data.getRevisionNumber()))
                    .withMethod(HttpMethod.GET.name())
                    .withQueryStringParameter("fields",
                            "id,vcs(type,from_branch,to_branch),active_diff_set(id),approvers"
                    )
            ).respond(response().withBody(JsonBody.json("""
                    {
                        "data": [ { "id": %d } ]
                    }""".formatted(data.getPullRequestId()))
            ));
        }
    }

    public void mockRegisterCiCheck() {
        server.when(request("/api/v1/diff-sets/4285430/ci-check").withMethod("PUT"))
                .respond(response().withStatusCode(HttpStatusCode.OK_200.code()));
    }

    public void verifyCreateReviewRequestComment(long pullRequestId, String comment) {
        verifyCreateReviewRequestComment(pullRequestId, comment, BodyVerificationMode.EQUAL);
    }

    public void verifyCreateReviewRequestComment(long pullRequestId, String comment, BodyVerificationMode mode) {
        verifyCreateReviewRequestComment(pullRequestId, comment, mode, atLeast(1));
    }

    public void verifyCreateReviewRequestComment(long pullRequestId, String comment, BodyVerificationMode mode,
                                                 VerificationTimes times) {
        var body = switch (mode) {
            case EQUAL -> JsonBody.json(toJson(new ArcanumClientImpl.ReviewRequestComment(comment)));
            case REGEXP -> RegexBody.regex(comment);
        };
        var request = request("/api/v1/review-requests/%d/comments".formatted(pullRequestId))
                .withMethod("POST")
                .withBody(body);
        server.verify(request, times);
    }

    public void verifySetMergeRequirementStatus(
            long pullRequestId,
            long diffSetId,
            UpdateCheckStatusRequest request
    ) {
        verifySetMergeRequirementStatus(pullRequestId, diffSetId, request, atLeast(1));
    }

    public void verifySetMergeRequirementStatus(
            long pullRequestId,
            long diffSetId,
            UpdateCheckStatusRequest request,
            VerificationTimes times
    ) {

        var httpRequest = request("/api/v1/review-requests/%d/diff-sets/%d/checks".formatted(pullRequestId, diffSetId))
                .withMethod("POST")
                .withBody(JsonBody.json(toJson(request)));
        server.verify(httpRequest, times);
    }

    public void reset() {
        server.reset();
    }

    public void clear(HttpRequest httpRequest) {
        server.clear(httpRequest);
    }

    public void verifyZeroInteractions() {
        server.verifyZeroInteractions();
    }

    public ClientAndServer getServer() {
        return server;
    }

    private static String toJson(Object obj) {
        try {
            return MAPPER.writeValueAsString(obj);
        } catch (JsonProcessingException e) {
            throw new UncheckedIOException(e);
        }
    }
}
