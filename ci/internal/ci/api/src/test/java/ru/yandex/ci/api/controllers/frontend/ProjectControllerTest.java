package ru.yandex.ci.api.controllers.frontend;

import java.time.Instant;
import java.util.List;

import com.google.protobuf.Int64Value;
import com.google.protobuf.StringValue;
import com.google.protobuf.Timestamp;
import io.grpc.ManagedChannel;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi;
import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi.GetConfigStatesRequest;
import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi.GetConfigStatesResponse;
import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi.GetProjectInfoResponse;
import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi.GetProjectRequest;
import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi.GetProjectResponse;
import ru.yandex.ci.api.internal.frontend.project.ProjectServiceGrpc;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.abc.AbcServiceEntity;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.proto.ProtoMappers;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static ru.yandex.ci.test.TestUtils.parseProtoText;

class ProjectControllerTest extends ControllerTestBase<ProjectServiceGrpc.ProjectServiceBlockingStub> {

    private static final int CLOCK_SECONDS = 77880;

    @Autowired
    private BranchService branchService;

    @Override
    protected ProjectServiceGrpc.ProjectServiceBlockingStub createStub(ManagedChannel channel) {
        return ProjectServiceGrpc.newBlockingStub(channel);
    }

    @Test
    void getProjectsOffsets() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(FrontendProjectApi.GetProjectsRequest.getDefaultInstance())
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("autocheck", "ci", "serpsearch", "testenv");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setLimit(1)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("autocheck");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setOffsetProjectId("ci")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("serpsearch", "testenv");
    }

    @Test
    void getProjectsWithDefaultFilter() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("rpsearc")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("serpsearch");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("rpsearc")
                                        .setLimit(1)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("serpsearch");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("rpsearc")
                                        .setOffsetProjectId("serpsearch")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).isEmpty();
    }

    @Test
    void getProjectsWithDefaultFilterDifferentCase() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("rpsEArc")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("serpsearch");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("rpsEArc")
                                        .setLimit(1)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("serpsearch");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("rpsEArc")
                                        .setOffsetProjectId("serpsearch")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).isEmpty();
    }

    @Test
    void getProjectsWithDefaultFilterMulti() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("e")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("autocheck", "serpsearch", "testenv");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("e")
                                        .setLimit(1)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("autocheck");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("e")
                                        .setOffsetProjectId("serpsearch")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("testenv");
    }

    @Test
    void getProjectsWithNameFilterWithoutDataInYqb() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("выдача")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).isEmpty();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("выдача")
                                        .setLimit(1)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).isEmpty();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("выдача")
                                        .setOffsetProjectId("serpsearch")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).isEmpty();
    }

    @Test
    void getProjectsWithNameFilter() {
        discoveryToR2();

        initAbcServicesTable();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("выдача")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("serpsearch");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("выдача")
                                        .setLimit(1)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("serpsearch");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("выдача")
                                        .setOffsetProjectId("serpsearch")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).isEmpty();
    }

    @Test
    void getProjectsWithNameFilterMatchInIdAnName() {
        discoveryToR2();

        initAbcServicesTable();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("check")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("autocheck", "ci");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("check")
                                        .setLimit(1)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("autocheck");

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setFilter("check")
                                        .setOffsetProjectId("autocheck")
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("ci");
    }

    @Test
    void getConfigStates() {
        discoveryToR2();
        clock.setTime(Instant.ofEpochSecond(CLOCK_SECONDS));

        delegateToken(TestData.SIMPLEST_RELEASE_PROCESS_ID.getPath());
        db.currentOrTx(() -> {
            branchService.createBranch(TestData.SIMPLEST_RELEASE_PROCESS_ID,
                    TestData.TRUNK_R2, TestData.CI_USER
            );
        });

        assertThat(grpcService.getConfigStates(
                GetConfigStatesRequest.newBuilder().setIncludeInvalidConfigs(false).build())
        ).isEqualTo(
                parseProtoText("project/getConfigStatesResponse.pb", GetConfigStatesResponse.class)
        );

        assertThat(grpcService.getConfigStates(
                GetConfigStatesRequest.newBuilder().setIncludeInvalidConfigs(true).build())
        ).isEqualTo(
                parseProtoText("project/getConfigStatesResponseWithInvalid.pb", GetConfigStatesResponse.class)
        );
    }

    @Test
    void getProjects_includeNotCiProject() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setIncludeInvalidConfigs(true)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("autocheck", "ci", "serpsearch", "testenv");
    }

    @Test
    void getProjectsData() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(FrontendProjectApi.GetProjectsRequest.getDefaultInstance())
        ).isEqualTo(
                FrontendProjectApi.GetProjectsResponse.newBuilder()
                        .addProjects(listProject(Abc.AUTOCHECK))
                        .addProjects(listProject(Abc.CI))
                        .addProjects(listProject(Abc.SERP_SEARCH))
                        .addProjects(listProject(Abc.TE))
                        .setOffset(Common.Offset.newBuilder().setTotal(Int64Value.of(4)).setHasMore(false).build())
                        .build()
        );
    }

    @Test
    void getProjectsCaseInsensitive() {
        discoveryToR2();

        abcServiceStub.removeService("testenv");
        assertThat(
                grpcService.getProjects(FrontendProjectApi.GetProjectsRequest.getDefaultInstance())
                        .getProjectsList()
        ).doesNotContain(listProject(Abc.TE));

        abcServiceStub.addService(Abc.TE);

        assertThat(
                grpcService.getProjects(FrontendProjectApi.GetProjectsRequest.getDefaultInstance())
                        .getProjectsList()
        ).contains(listProject(Abc.TE));
    }

    @Test
    void getProject() {
        discoveryToR2();
        clock.setTime(Instant.ofEpochSecond(CLOCK_SECONDS));

        delegateToken(TestData.EMPTY_RELEASE_PROCESS_ID.getPath());
        var branch = db.currentOrTx(() -> branchService.createBranch(TestData.EMPTY_RELEASE_PROCESS_ID,
                TestData.TRUNK_R2, TestData.CI_USER
        ));
        db.currentOrTx(() -> {
            registerCancelledInBranch(branch, 19, TestData.TRUNK_R2);
            registerCancelledInBranch(branch, 20, TestData.RELEASE_R6_1);
            registerCancelledInBranch(branch, 29, TestData.RELEASE_R6_3);
        });
        assertThat(
                grpcService.getProject(
                        GetProjectRequest.newBuilder()
                                .setProjectId("ci")
                                .build()
                )
        ).isEqualTo(parseProtoText("project/getProject.pb", GetProjectResponse.class));

        assertThat(
                grpcService.getProject(
                        GetProjectRequest.newBuilder()
                                .setProjectId("serpsearch")
                                .build()
                )
        ).isEqualTo(parseProtoText("project/getProjectNotCi.pb", GetProjectResponse.class));

        assertThat(
                grpcService.getProject(
                        GetProjectRequest.newBuilder()
                                .setProjectId("serpsearch")
                                .setIncludeInvalidConfigs(true)
                                .build()
                )
        ).isEqualTo(parseProtoText("project/getProjectNotCi.IncludeInvalid.pb", GetProjectResponse.class));

        assertThat(
                grpcService.getProjectInfo(FrontendProjectApi.GetProjectInfoRequest.newBuilder()
                        .setProjectId("ci")
                        .setIncludeInvalidConfigs(true)
                        .build())
        ).isEqualTo(parseProtoText("project/getProjectInfo.pb", GetProjectInfoResponse.class));
    }

    @Test
    void getConfigHistory() {
        discoveryToR4();

        FrontendProjectApi.ConfigEntity r1Entity = configEntity(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_R2,
                true,
                false,
                "No token was found for this configuration"
        );
        FrontendProjectApi.ConfigEntity r2Entity = configEntity(
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_R3,
                false,
                false,
                "Invalid config",
                configProblem(
                        "Invalid a.yaml configuration",
                        """
                                {
                                  "level" : "error",
                                  "schema" : {
                                    "loadingURI" : "#",
                                    "pointer" : "/properties/ci"
                                  },
                                  "instance" : {
                                    "pointer" : "/ci"
                                  },
                                  "domain" : "validation",
                                  "keyword" : "additionalProperties",
                                  "message" : "object instance has properties which are not allowed by the schema: \
                                [\\"invalid-config\\"]",
                                  "unwanted" : [ "invalid-config" ]
                                }"""
                ),
                configProblem(
                        "Invalid a.yaml configuration",
                        """
                                {
                                  "level" : "error",
                                  "schema" : {
                                    "loadingURI" : "#",
                                    "pointer" : "/properties/ci"
                                  },
                                  "instance" : {
                                    "pointer" : "/ci"
                                  },
                                  "domain" : "validation",
                                  "keyword" : "dependencies",
                                  "message" : "property \\"secret\\" of object has missing property dependencies \
                                (schema requires [\\"runtime\\"]; missing: [\\"runtime\\"])",
                                  "property" : "secret",
                                  "required" : [ "runtime" ],
                                  "missing" : [ "runtime" ]
                                }"""
                )
        );
        FrontendProjectApi.ConfigEntity r3Entity = configEntity(
                TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_R4,
                true,
                false,
                "No token was found for this configuration"
        );

        String dir = TestData.CONFIG_PATH_ABC.getParent().toString();

        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(dir)
                                .build()
                )
        ).isEqualTo(historyResponse(r3Entity, r3Entity, r2Entity, r1Entity));


        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(dir)
                                .setLimit(2)
                                .build()
                )
        ).isEqualTo(historyResponse(r3Entity, offset(3, true), r3Entity, r2Entity));

        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(dir)
                                .setOffsetCommitNumber(TestData.TRUNK_R4.getNumber())
                                .build()
                )
        ).isEqualTo(historyResponse(r3Entity, offset(3, false), r2Entity, r1Entity));

        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(dir)
                                .setLimit(1)
                                .setOffsetCommitNumber(TestData.TRUNK_R4.getNumber())
                                .build()
                )
        ).isEqualTo(historyResponse(r3Entity, offset(3, true), r2Entity));
    }

    @Test
    void getConfigHistory_forPrBranch_whenPrDoesNotAffectConfig() {
        discoveryToR4();
        initDiffSets();

        assertThat(configurationService
                .getOrCreateConfig(TestData.CONFIG_PATH_CHANGE_DS1, TestData.DIFF_SET_1.getOrderedMergeRevision()))
                .isNotEmpty();
        assertThat(configurationService
                .getOrCreateConfig(TestData.CONFIG_PATH_CHANGE_DS1, TestData.DIFF_SET_2.getOrderedMergeRevision()))
                .isNotEmpty();
        assertThat(configurationService
                .getOrCreateConfig(TestData.CONFIG_PATH_CHANGE_DS1, TestData.DIFF_SET_3.getOrderedMergeRevision()))
                .isNotEmpty();

        FrontendProjectApi.ConfigEntity r1Entity = configEntity(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_R2,
                true,
                false,
                "No token was found for this configuration"
        );
        FrontendProjectApi.ConfigEntity r2Entity = configEntity(
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_R3,
                true,
                false,
                "No token was found for this configuration"
        );

        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(TestData.CONFIG_PATH_CHANGE_DS1.getParent().toString())
                                .setBranch(StringValue.of("pr:42"))
                                .setLimit(10)
                                .build()
                )
        ).isEqualTo(
                FrontendProjectApi.GetConfigHistoryResponse.newBuilder()
                        .addAllConfigEntities(List.of(
                                /* DIFF_SET_3 - is a merge revision, that doesn't affect
                                TestData.CONFIG_PATH_CHANGE_DS1 path, so result doesn't contain
                                config at DS3_REVISION */
                                r2Entity,
                                r1Entity
                        ))
                        .setLastValidEntity(r2Entity)
                        .setOffset(offset(2, false))
                        .build()
        );
    }


    @Test
    void getConfigHistory_forPrBranch_whenPrAffectsConfig() {
        discoveryToR4();
        initDiffSets();

        assertThat(configurationService
                .getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.DIFF_SET_1.getOrderedMergeRevision()))
                .isNotEmpty();
        assertThat(configurationService
                .getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.DIFF_SET_3.getOrderedMergeRevision()))
                .isNotEmpty();

        FrontendProjectApi.ConfigEntity r3Entity = configEntity(
                TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_R4,
                true,
                false,
                "No token was found for this configuration"
        );
        FrontendProjectApi.ConfigEntity r2Entity = configEntity(
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_R3,
                false,
                false,
                "Invalid config",
                configProblem(
                        "Invalid a.yaml configuration",
                        """
                                {
                                  "level" : "error",
                                  "schema" : {
                                    "loadingURI" : "#",
                                    "pointer" : "/properties/ci"
                                  },
                                  "instance" : {
                                    "pointer" : "/ci"
                                  },
                                  "domain" : "validation",
                                  "keyword" : "additionalProperties",
                                  "message" : "object instance has properties which are not allowed by the schema: \
                                [\\"invalid-config\\"]",
                                  "unwanted" : [ "invalid-config" ]
                                }"""
                ),
                configProblem(
                        "Invalid a.yaml configuration",
                        """
                                {
                                  "level" : "error",
                                  "schema" : {
                                    "loadingURI" : "#",
                                    "pointer" : "/properties/ci"
                                  },
                                  "instance" : {
                                    "pointer" : "/ci"
                                  },
                                  "domain" : "validation",
                                  "keyword" : "dependencies",
                                  "message" : "property \\"secret\\" of object has missing property dependencies \
                                (schema requires [\\"runtime\\"]; missing: [\\"runtime\\"])",
                                  "property" : "secret",
                                  "required" : [ "runtime" ],
                                  "missing" : [ "runtime" ]
                                }"""
                )
        );
        FrontendProjectApi.ConfigEntity r1Entity = configEntity(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_R2,
                true,
                false,
                "No token was found for this configuration"
        );

        FrontendProjectApi.ConfigEntity ds3Entity = FrontendProjectApi.ConfigEntity.newBuilder()
                .setCommit(Common.Commit.newBuilder()
                        .setAuthor(TestData.DS3_COMMIT.getAuthor())
                        .setMessage(TestData.DS3_COMMIT.getMessage())
                        .setDate(ProtoConverter.convert(TestData.COMMIT_DATE))
                        .setRevision(
                                Common.OrderedArcRevision.newBuilder()
                                        .setNumber(3)
                                        .setHash(TestData.DS3_COMMIT.getCommitId())
                                        .setBranch(ArcBranch.ofPullRequest(42).asString())
                                        .setPullRequestId(42)
                        )
                )
                .setValid(true)
                .setHasToken(false)
                .setTokenStatus("No token was found for this configuration")
                .addAllProblems(List.of())
                .build();

        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(TestData.CONFIG_PATH_ABC.getParent().toString())
                                .setBranch(StringValue.of("pr:42"))
                                .build()
                )
        ).isEqualTo(
                FrontendProjectApi.GetConfigHistoryResponse.newBuilder()
                        .addAllConfigEntities(List.of(
                                ds3Entity, r3Entity, r2Entity, r1Entity
                        ))
                        .setOffset(offset(4, false))
                        .setLastValidEntity(ds3Entity)
                        .build()
        );

        // test offset
        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(TestData.CONFIG_PATH_ABC.getParent().toString())
                                .setBranch(StringValue.of("pr:42"))
                                .setLimit(1)
                                .build()
                )
        ).isEqualTo(
                FrontendProjectApi.GetConfigHistoryResponse.newBuilder()
                        .addAllConfigEntities(List.of(
                                ds3Entity
                        ))
                        .setOffset(offset(4, true))
                        .setLastValidEntity(ds3Entity)
                        .build()
        );
        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(TestData.CONFIG_PATH_ABC.getParent().toString())
                                .setBranch(StringValue.of("pr:42"))
                                .setOffsetCommitNumber(TestData.DIFF_SET_3.getDiffSetId())
                                // merge commit
                                .setOffsetCommitHash(StringValue.of(TestData.DS3_COMMIT.getCommitId()))
                                .setLimit(1)
                                .build()
                )
        ).isEqualTo(
                FrontendProjectApi.GetConfigHistoryResponse.newBuilder()
                        .addAllConfigEntities(List.of(
                                r3Entity
                        ))
                        .setOffset(offset(4, true))
                        .setLastValidEntity(ds3Entity)
                        .build()
        );
    }

    @Test
    void getConfigHistory_forReleaseBranch() {
        var processId = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;
        discoveryToR2();
        delegateToken(processId.getPath());

        doReturn("releases/ci/release-component-1")
                .when(branchNameGenerator)
                .generateName(any(), any(), anyInt());

        var branch1 = createBranchAt(TestData.TRUNK_COMMIT_2, processId).getArcBranch();
        assertThat(branch1.asString()).isEqualTo("releases/ci/release-component-1");

        discoverCommits(branch1, TestData.RELEASE_R2_1);

        assertThat(configurationService
                .getOrCreateConfig(processId.getPath(), TestData.RELEASE_R2_1))
                .isNotEmpty();

        FrontendProjectApi.ConfigEntity r1Entity = configEntity(
                TestData.RELEASE_BRANCH_COMMIT_2_1,
                TestData.RELEASE_R2_1,
                true,
                false,
                "User has no rights to edit configuration"
        );

        // Last valid config we use to create a branch
        FrontendProjectApi.ConfigEntity rTrunkEntity = configEntity(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_R2,
                true,
                true,
                "New token delegated for this revision"
        );

        assertThat(
                grpcService.getConfigHistory(
                        FrontendProjectApi.GetConfigHistoryRequest.newBuilder()
                                .setConfigDir(processId.getDir())
                                .setBranch(StringValue.of(branch1.toString()))
                                .setLimit(10)
                                .build()
                )
        ).isEqualTo(
                FrontendProjectApi.GetConfigHistoryResponse.newBuilder()
                        .addAllConfigEntities(List.of(r1Entity, rTrunkEntity))
                        .setLastValidEntity(r1Entity)
                        .setOffset(offset(2, false))
                        .build()
        );
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void getFavoriteProjects() {
        discoveryToR2();

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setOnlyFavorite(true)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).isEmpty();

        grpcService.addFavoriteProject(
                FrontendProjectApi.AddFavoriteProjectRequest.newBuilder().setProjectId("ci").build()
        );

        grpcService.addFavoriteProject(
                FrontendProjectApi.AddFavoriteProjectRequest.newBuilder().setProjectId("testenv").build()
        );

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setOnlyFavorite(true)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("ci", "testenv");

        grpcService.removeFavoriteProject(
                FrontendProjectApi.RemoveFavoriteProjectRequest.newBuilder().setProjectId("testenv").build()
        );

        assertThat(
                grpcService.getProjects(
                                FrontendProjectApi.GetProjectsRequest.newBuilder()
                                        .setOnlyFavorite(true)
                                        .build()
                        )
                        .getProjectsList()
                        .stream()
                        .map(FrontendProjectApi.ListProject::getProject)
                        .map(Common.Project::getId)
        ).containsExactly("ci");


    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void getProjectFavoriteMark() {
        discoveryToR2();
        grpcService.addFavoriteProject(
                FrontendProjectApi.AddFavoriteProjectRequest.newBuilder().setProjectId("ci").build()
        );

        assertThat(
                grpcService.getProjects(FrontendProjectApi.GetProjectsRequest.getDefaultInstance())
        ).isEqualTo(
                FrontendProjectApi.GetProjectsResponse.newBuilder()
                        .addProjects(listProject(Abc.AUTOCHECK, false))
                        .addProjects(listProject(Abc.CI, true))
                        .addProjects(listProject(Abc.SERP_SEARCH, false))
                        .addProjects(listProject(Abc.TE, false))
                        .setOffset(Common.Offset.newBuilder().setTotal(Int64Value.of(4)).setHasMore(false).build())
                        .build()
        );

        assertThat(
                grpcService.getProjects(
                        FrontendProjectApi.GetProjectsRequest.newBuilder().
                                setOnlyFavorite(true)
                                .build()
                )
        ).isEqualTo(
                FrontendProjectApi.GetProjectsResponse.newBuilder()
                        .addProjects(listProject(Abc.CI, true))
                        .setOffset(Common.Offset.newBuilder().setTotal(Int64Value.of(1)).setHasMore(false).build())
                        .build()
        );
    }

    private void initAbcServicesTable() {
        // Cache everything in YDB
        // TODO: remove after removing AbcServiceStub
        var services = abcServiceStub.getServices().stream()
                .map(info -> AbcServiceEntity.of(info, clock.instant()))
                .toList();
        db.currentOrTx(() -> db.abcServices().save(services));
    }

    private void registerCancelledInBranch(Branch branch, int number, OrderedArcRevision revision) {
        branchService.updateTimelineBranchAndLaunchItems(Launch.builder()
                .id(Launch.Id.of(branch.getProcessId().asString(), number))
                .flowInfo(LaunchFlowInfo.builder()
                        .configRevision(TestData.TRUNK_R4)
                        .flowId(FlowFullId.of(branch.getProcessId().getPath(), branch.getProcessId().getSubId()))
                        .stageGroupId("my-stages")
                        .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                        .build())
                .vcsInfo(LaunchVcsInfo.builder()
                        .selectedBranch(branch.getArcBranch())
                        .revision(revision)
                        .build())
                .status(LaunchState.Status.CANCELED)
                .build());
    }


    private Common.Offset offset(int total, boolean hasMore) {
        return Common.Offset.newBuilder()
                .setTotal(Int64Value.of(total))
                .setHasMore(hasMore)
                .build();
    }


    private FrontendProjectApi.GetConfigHistoryResponse historyResponse(FrontendProjectApi.ConfigEntity lastValidEntity,
                                                                        FrontendProjectApi.ConfigEntity... entities) {
        Common.Offset page = Common.Offset.newBuilder()
                .setTotal(Int64Value.of(entities.length))
                .setHasMore(false)
                .build();
        return historyResponse(lastValidEntity, page, entities);
    }

    private FrontendProjectApi.GetConfigHistoryResponse historyResponse(FrontendProjectApi.ConfigEntity lastValidEntity,
                                                                        Common.Offset offset,
                                                                        FrontendProjectApi.ConfigEntity... entities) {
        return FrontendProjectApi.GetConfigHistoryResponse.newBuilder()
                .setLastValidEntity(lastValidEntity)
                .addAllConfigEntities(List.of(entities))
                .setOffset(offset)
                .build();
    }

    private static FrontendProjectApi.ConfigProblem configProblem(String title, String description) {
        return FrontendProjectApi.ConfigProblem.newBuilder()
                .setTitle(title)
                .setDescription(description)
                .setLevel(FrontendProjectApi.ConfigProblem.Level.CRIT)
                .build();
    }


    private static FrontendProjectApi.ConfigEntity configEntity(
            ArcCommit commit,
            OrderedArcRevision arcRevision,
            boolean valid,
            boolean hasToken,
            String tokenStatus,
            FrontendProjectApi.ConfigProblem... problems) {
        return FrontendProjectApi.ConfigEntity.newBuilder()
                .setCommit(toCommit(commit, arcRevision))
                .setValid(valid)
                .setHasToken(hasToken)
                .setTokenStatus(tokenStatus)
                .addAllProblems(List.of(problems))
                .build();
    }

    private static Common.Commit toCommit(ArcCommit commit, OrderedArcRevision revision) {
        return Common.Commit.newBuilder()
                .setAuthor(commit.getAuthor())
                .setMessage(commit.getMessage())
                .setDate(
                        Timestamp.newBuilder()
                                .setSeconds(TestData.COMMIT_DATE.getEpochSecond())
                                .setNanos(TestData.COMMIT_DATE.getNano())
                                .build()
                )
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();
    }

    public static Common.Project project(Abc abc) {
        return project(abc, false);
    }

    public static FrontendProjectApi.ListProject listProject(Abc abc) {
        return listProject(abc, false);
    }

    public static FrontendProjectApi.ListProject listProject(Abc abc, boolean favorite) {
        return FrontendProjectApi.ListProject.newBuilder()
                .setIsFavorite(favorite)
                .setProject(project(abc, favorite))
                .addAllAbcHierarchy(abc.getHierarchy().isEmpty()
                        ? List.of(
                        Common.AbcService.newBuilder()
                                .setName(Common.LocalizedName.newBuilder()
                                        .setRu("Корневой сервис")
                                        .setEn("Root"))
                                .setDescription(Common.LocalizedName.newBuilder()
                                        .setRu("Корневой сервис")
                                        .setEn("Root"))
                                .setUrl("https://abc.yandex-team.ru/services/")
                                .build())
                        :
                        abc.getHierarchy().stream()
                                .map(ProjectControllerTest::abcService)
                                .toList()
                )
                .build();
    }

    public static Common.Project project(Abc abc, boolean favorite) {
        return Common.Project.newBuilder()
                .setId(abc.getSlug())
                .setAbcService(abcService(abc))
                .build();
    }

    public static Common.AbcService abcService(Abc abc) {
        return Common.AbcService.newBuilder()
                .setSlug(abc.getSlug())
                .setName(Common.LocalizedName.newBuilder().setRu("Название").setEn(abc.getName()).build())
                .setDescription(Common.LocalizedName.newBuilder().setRu("Описание").setEn("Description").build())
                .setUrl("https://abc.yandex-team.ru/services/" + abc.getSlug().toLowerCase())
                .build();
    }

}
