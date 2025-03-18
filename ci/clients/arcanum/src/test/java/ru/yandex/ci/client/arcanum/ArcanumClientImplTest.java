package ru.yandex.ci.client.arcanum;

import java.nio.file.Path;
import java.util.List;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.model.JsonBody;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.util.ResourceUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.HttpStatusCode.NO_CONTENT_204;
import static org.mockserver.model.HttpStatusCode.OK_200;

@ExtendWith(MockitoExtension.class)
@ExtendWith(MockServerExtension.class)
class ArcanumClientImplTest {

    private final MockServerClient server;
    private ArcanumClientImpl arcanumClient;

    ArcanumClientImplTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();

        var properties = HttpClientProperties.builder()
                .endpoint("http:/" + server.remoteAddress())
                .retryPolicy(ArcanumClient.defaultRetryPolicy())
                .build();
        arcanumClient = ArcanumClientImpl.create(properties, false);
    }

    @Test
    void generateUrlForArcPathAtTrunkHead() {
        assertThat(arcanumClient.generateUrlForArcPathAtTrunkHead(Path.of("ci/a.yaml")))
                .isEqualTo("/arc_vcs/ci/a.yaml");
    }

    @Test
    void setMergeRequirementsWithCheckingActiveDiffSet() {
        server.when(request("/api/v1/review-requests/1/check-requirements")
                .withMethod(HttpMethod.PATCH.name())
                .withQueryStringParameter("diff_set_id", "100")
                .withBody(JsonBody.json("""
                        [{
                            "system": "CI",
                            "type": "CHECK_1",
                            "enabled": false,
                            "reason": { "text": "some reason" }
                        }]"""
                )))
                .respond(response().withStatusCode(NO_CONTENT_204.code()));

        arcanumClient.setMergeRequirementsWithCheckingActiveDiffSet(1L, 100L, List.of(
                new UpdateCheckRequirementRequestDto(
                        "CI", "CHECK_1", false,
                        new UpdateCheckRequirementRequestDto.ReasonDto("some reason")
                )
        ));
    }

    @Test
    void setMergeRequirement() {
        server.when(request("/api/v1/review-requests/1/checks")
                .withMethod(HttpMethod.POST.name())
                .withBody(JsonBody.json("""
                        {
                            "system": "ci",
                            "type": "tests",
                            "required": true
                        }"""
                )))
                .respond(response().withStatusCode(NO_CONTENT_204.code()));

        arcanumClient.setMergeRequirement(
                1L,
                ArcanumMergeRequirementId.of("ci", "tests"),
                ArcanumClient.Require.required()
        );
    }

    @Test
    void setMergeRequirementStatus() {
        server.when(request("/api/v1/review-requests/1/diff-sets/100/checks")
                .withMethod(HttpMethod.POST.name())
                .withBody(JsonBody.json("""
                        {
                            "system": "ci",
                            "type": "tests",
                            "status": "success",
                            "description": "Test description",
                            "system_check_uri": "http://a.yandex-team.ru/..."
                        }"""
                )))
                .respond(response().withStatusCode(NO_CONTENT_204.code()));

        arcanumClient.setMergeRequirementStatus(1L, 100L,
                ArcanumMergeRequirementId.of("ci", "tests"),
                ArcanumMergeRequirementDto.Status.SUCCESS,
                "http://a.yandex-team.ru/...",
                "Test description"
        );
    }

    @Test
    void createReviewRequestComment() {
        server.when(request("/api/v1/review-requests/1/comments")
                .withMethod(HttpMethod.POST.name())
                .withBody(JsonBody.json("""
                        {
                            "content": "This is a comment"
                        }"""
                )))
                .respond(response().withStatusCode(NO_CONTENT_204.code()));

        arcanumClient.createReviewRequestComment(1L, "This is a comment");
    }

    @Test
    void getReviewRequestBySvnRevision() {
        server.when(request("/api/v1/merge-commits/8622193/review-requests")
                .withMethod(HttpMethod.GET.name())
                .withQueryStringParameter("fields", "id,vcs(type,from_branch,to_branch),active_diff_set(id),approvers"))
                .respond(response()
                        .withBody(JsonBody.json("""
                                {
                                  "data" : [ {
                                    "id" : 2012763,
                                    "vcs" : {
                                      "type" : "arc",
                                      "from_branch" : "users/albazh/auto-generated-1631533435",
                                      "to_branch" : "trunk"
                                    },
                                    "approvers" : [ {
                                      "name" : "albazh"
                                    } ],
                                    "active_diff_set" : {
                                      "id" : 4323828
                                    }
                                  } ]
                                }"""
                        ))
                        .withStatusCode(OK_200.code()));
        assertThat(arcanumClient.getReviewRequestBySvnRevision(8622193).get())
                .isEqualTo(ArcanumReviewDataDto.builder()
                        .id(2012763L)
                        .vcs(new ArcanumReviewDataDto.Vcs("arc", "users/albazh/auto-generated-1631533435", "trunk"))
                        .activeDiffSet(ArcanumReviewDataDto.DiffSet.of(4323828L))
                        .approvers(List.of(new ArcanumReviewDataDto.Person("albazh")))
                        .build()
                );
    }

    @Test
    void getReviewRequestData() {
        var fields =
                "id,active_diff_set(id),vcs(type,from_branch,to_branch),checks(system,type,required,satisfied,status," +
                        "disabling_policy,system_check_uri,system_check_id)";

        server.when(request("/api/v1/review-requests/2011985")
                .withQueryStringParameter("fields", fields)
                .withMethod(HttpMethod.GET.name()))
                .respond(response()
                        .withBody(JsonBody.json(resource("getReviewRequestData-response.json")))
                        .withStatusCode(OK_200.code()));

        assertThat(arcanumClient.getReviewRequestData(2011985L, fields).get())
                .isEqualTo(ArcanumReviewDataDto.builder()
                        .id(2011985L)
                        .vcs(new ArcanumReviewDataDto.Vcs(
                                "arc",
                                "users/swnet/arcanum-93281d2b-9c73-4343-9e48-a89f0ff4ff29",
                                "releases/kinopoisk-frontend/ott-hd/v2.219"
                        ))
                        .checks(List.of(
                                new ArcanumMergeRequirementDto(
                                        "CI", "[validation] kinopoisk/frontend/ott/a.yaml", true, true,
                                        ArcanumMergeRequirementDto.Status.SUCCESS,
                                        "https://a.yandex-team.ru/", "some-external-id", DisablingPolicyDto.ALLOWED,
                                        null
                                ),
                                new ArcanumMergeRequirementDto(
                                        "arcanum", "approved", false, false,
                                        ArcanumMergeRequirementDto.Status.FAILURE,
                                        null, null, DisablingPolicyDto.NEED_REASON, null
                                )
                        ))
                        .activeDiffSet(ArcanumReviewDataDto.DiffSet.of(4321674L))
                        .build());
    }

    @Test
    void registerCiCheck() {
        server.when(request("/api/v1/diff-sets/10/ci-check")
                .withMethod(HttpMethod.PUT.name())
                .withBody(JsonBody.json("""
                        {
                            "check_id": "100",
                            "fast_circuit": false,
                            "new_plate": true
                        }"""
                )))
                .respond(response().withStatusCode(NO_CONTENT_204.code()));

        arcanumClient.registerCiCheck(
                10L, new RegisterCheckRequestDto(Long.toString(100L), false, true)
        );
    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }

}
