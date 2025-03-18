package ru.yandex.ci.ayamler.api.controllers;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import io.grpc.ManagedChannel;
import org.junit.jupiter.api.Test;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.ayamler.AYaml;
import ru.yandex.ci.ayamler.AYamlerService;
import ru.yandex.ci.ayamler.AYamlerServiceGrpc;
import ru.yandex.ci.ayamler.AYamlerServiceGrpc.AYamlerServiceBlockingStub;
import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.ayamler.PathAndLogin;
import ru.yandex.ci.ayamler.PathNotFoundException;
import ru.yandex.ci.ayamler.StrongMode;
import ru.yandex.ci.ayamler.api.spring.AYamlerApiConfig;
import ru.yandex.ci.ayamler.api.spring.AbcClientTestConfig;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;

@ContextConfiguration(classes = {
        AYamlerApiConfig.class,

        AbcClientTestConfig.class,
        ArcClientTestConfig.class
})
public class AYamlerControllerTest extends ControllerTestBase<AYamlerServiceBlockingStub> {

    @MockBean
    AYamlerService aYamlerService;

    @Test
    void getStrongMode_whenLoginIsEmpty() {
        doReturn(Optional.of(
                AYaml.valid(
                        Path.of("ci/a.yaml"), "ci",
                        new StrongMode(true, Set.of()),
                        false
                )
        )).when(aYamlerService).getStrongMode(any(), any(), isNull());
        var response = grpcClient.getStrongMode(
                Ayamler.GetStrongModeRequest.newBuilder()
                        .setRevision("30276f96b118fa805cf50e1fd22713bee2fb665e")
                        .setPath("ci/tms/src/main/java")
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetStrongModeResponse.newBuilder()
                        .setStrongMode(Ayamler.StrongMode.newBuilder()
                                .setRevision("30276f96b118fa805cf50e1fd22713bee2fb665e")
                                .setPath("ci/tms/src/main/java")
                                .setStatus(Ayamler.StrongModeStatus.ON)
                                .setAyaml(Ayamler.AYaml.newBuilder()
                                        .setPath("ci/a.yaml")
                                        .setService("ci")
                                        .setValid(true)
                                        .build()
                                )
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getStrongMode_whenLoginNotEmpty() {
        doReturn(Optional.of(
                AYaml.valid(
                        Path.of("ci/a.yaml"), "ci",
                        new StrongMode(true, Set.of()),
                        false
                )
        )).when(aYamlerService).getStrongMode(any(), any(), anyString());
        var response = grpcClient.getStrongMode(
                Ayamler.GetStrongModeRequest.newBuilder()
                        .setRevision("30276f96b118fa805cf50e1fd22713bee2fb665e")
                        .setPath("ci/tms/src/main/java")
                        .setLogin("check-author")
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetStrongModeResponse.newBuilder()
                        .setStrongMode(Ayamler.StrongMode.newBuilder()
                                .setRevision("30276f96b118fa805cf50e1fd22713bee2fb665e")
                                .setPath("ci/tms/src/main/java")
                                .setLogin("check-author")
                                .setStatus(Ayamler.StrongModeStatus.ON)
                                .setAyaml(Ayamler.AYaml.newBuilder()
                                        .setPath("ci/a.yaml")
                                        .setValid(true)
                                        .setService("ci")
                                        .build()
                                )
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getStrongMode_whenSvnRevisionRequested() {
        doReturn(Optional.of(
                AYaml.valid(
                        Path.of("ci/a.yaml"), "ci",
                        new StrongMode(true, Set.of()),
                        false
                )
        )).when(aYamlerService).getStrongMode(any(), any(), anyString());
        var response = grpcClient.getStrongMode(
                Ayamler.GetStrongModeRequest.newBuilder()
                        .setRevision("100")
                        .setPath("ci/tms/src/main/java")
                        .setLogin("check-author")
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetStrongModeResponse.newBuilder()
                        .setStrongMode(Ayamler.StrongMode.newBuilder()
                                // check that controller doesn't return normalized svn revision "r100"
                                .setRevision("100")
                                .setPath("ci/tms/src/main/java")
                                .setLogin("check-author")
                                .setStatus(Ayamler.StrongModeStatus.ON)
                                .setAyaml(Ayamler.AYaml.newBuilder()
                                        .setPath("ci/a.yaml")
                                        .setValid(true)
                                        .setService("ci")
                                        .build()
                                )
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getStrongMode_whenRequestedPathNotFound() {
        doThrow(new PathNotFoundException(
                ArcRevision.of("a".repeat(40)), Path.of("path/////does-not-exist")
        )).when(aYamlerService).getStrongMode(any(), any(), anyString());
        var response = grpcClient.getStrongMode(
                Ayamler.GetStrongModeRequest.newBuilder()
                        .setRevision("a".repeat(40))
                        .setPath("path/////does-not-exist")
                        .setLogin("check-author")
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetStrongModeResponse.newBuilder()
                        .setStrongMode(Ayamler.StrongMode.newBuilder()
                                .setRevision("a".repeat(40))
                                // check that controller doesn't return normalized path in response
                                .setPath("path/////does-not-exist")
                                .setLogin("check-author")
                                .setStatus(Ayamler.StrongModeStatus.NOT_FOUND)
                                .setAyaml(
                                        Ayamler.AYaml.newBuilder()
                                                .setService("autocheck")
                                                .build()
                                )
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getStrongModeBatch_whenLoginIsEmpty() {
        doReturn(Map.of(
                PathAndLogin.of(Path.of("path1"), null), Optional.of(AYaml.valid(
                        Path.of("ci/a.yaml"), "ci", new StrongMode(true, Set.of()), false
                )),
                PathAndLogin.of(Path.of("path2"), null), Optional.of(AYaml.valid(
                        Path.of("ci/a.yaml"), "ci", new StrongMode(true, Set.of()), false
                ))
        )).when(aYamlerService).getStrongMode(
                eq(ArcRevision.of("a".repeat(40))),
                eq(Set.of(
                        PathAndLogin.of(Path.of("path1"), null),
                        PathAndLogin.of(Path.of("path2"), null)
                ))
        );
        doReturn(Optional.of(AYaml.valid(
                Path.of("ci/a.yaml"), "ci", new StrongMode(true, Set.of()), false
        ))).when(aYamlerService).getStrongMode(any(), any(), anyString());
        var response = grpcClient.getStrongModeBatch(
                Ayamler.GetStrongModeBatchRequest.newBuilder()
                        .addAllRequest(List.of(
                                Ayamler.GetStrongModeRequest.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path1")
                                        .build(),
                                Ayamler.GetStrongModeRequest.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path2")
                                        .build()
                        ))
                        .build()
        );

        assertThat(response.getStrongModeList()).containsAll(
                List.of(
                        Ayamler.StrongMode.newBuilder()
                                .setRevision("a".repeat(40))
                                .setPath("path1")
                                .setStatus(Ayamler.StrongModeStatus.ON)
                                .setAyaml(Ayamler.AYaml.newBuilder()
                                        .setPath("ci/a.yaml")
                                        .setService("ci")
                                        .setValid(true)
                                        .build()
                                )
                                .build(),
                        Ayamler.StrongMode.newBuilder()
                                .setRevision("a".repeat(40))
                                .setPath("path2")
                                .setStatus(Ayamler.StrongModeStatus.ON)
                                .setAyaml(Ayamler.AYaml.newBuilder()
                                        .setPath("ci/a.yaml")
                                        .setService("ci")
                                        .setValid(true)
                                        .build()
                                )
                                .build()
                )
        );
    }

    @Test
    void getStrongModeBatch_whenLoginNotEmpty() {
        doReturn(Map.of(
                PathAndLogin.of(Path.of("path1"), "login1"), Optional.of(AYaml.valid(
                        Path.of("ci/a.yaml"), "ci", new StrongMode(true, Set.of()), false
                )),
                PathAndLogin.of(Path.of("path2"), "login2"), Optional.of(AYaml.valid(
                        Path.of("ci/a.yaml"), "ci", new StrongMode(true, Set.of()), false
                ))
        )).when(aYamlerService).getStrongMode(
                eq(ArcRevision.of("a".repeat(40))),
                eq(Set.of(
                        PathAndLogin.of(Path.of("path1"), "login1"),
                        PathAndLogin.of(Path.of("path2"), "login2")
                ))
        );
        doReturn(Optional.of(AYaml.valid(
                Path.of("ci/a.yaml"), "ci", new StrongMode(true, Set.of()), false
        ))).when(aYamlerService).getStrongMode(any(), any(), anyString());
        var response = grpcClient.getStrongModeBatch(
                Ayamler.GetStrongModeBatchRequest.newBuilder()
                        .addAllRequest(List.of(
                                Ayamler.GetStrongModeRequest.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path1")
                                        .setLogin("login1")
                                        .build(),
                                Ayamler.GetStrongModeRequest.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path2")
                                        .setLogin("login2")
                                        .build()
                        ))
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetStrongModeBatchResponse.newBuilder()
                        .addAllStrongMode(List.of(
                                Ayamler.StrongMode.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path1")
                                        .setLogin("login1")
                                        .setStatus(Ayamler.StrongModeStatus.ON)
                                        .setAyaml(Ayamler.AYaml.newBuilder()
                                                .setPath("ci/a.yaml")
                                                .setValid(true)
                                                .setService("ci")
                                                .build()
                                        )
                                        .build(),
                                Ayamler.StrongMode.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path2")
                                        .setLogin("login2")
                                        .setStatus(Ayamler.StrongModeStatus.ON)
                                        .setAyaml(Ayamler.AYaml.newBuilder()
                                                .setPath("ci/a.yaml")
                                                .setValid(true)
                                                .setService("ci")
                                                .build()
                                        )
                                        .build()
                        ))
                        .build()
        );
    }

    @Test
    void getAbcServiceSlugBatch() {
        doReturn(Map.of(
                Path.of("ci/tms/src/main/java"),
                Optional.of(
                        AYaml.valid(Path.of("ci/a.yaml"), "ci", new StrongMode(false, Set.of()), false)
                )
        )).when(aYamlerService).getAbcServiceSlugBatch(any(), any());

        var response = grpcClient.getAbcServiceSlugBatch(
                Ayamler.GetAbcServiceSlugBatchRequest.newBuilder()
                        .addRequest(Ayamler.GetAbcServiceSlugRequest.newBuilder()
                                .setRevision("30276f96b118fa805cf50e1fd22713bee2fb665e")
                                .setPath("ci/tms/src/main/java"))
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetAbcServiceSlugBatchResponse.newBuilder()
                        .addAbcService(Ayamler.AbcService.newBuilder()
                                .setRevision("30276f96b118fa805cf50e1fd22713bee2fb665e")
                                .setPath("ci/tms/src/main/java")
                                .setAyaml(Ayamler.AYaml.newBuilder()
                                        .setPath("ci/a.yaml")
                                        .setValid(true)
                                        .setService("ci")
                                        .build()
                                )
                                .setSlug("ci")
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getAbcServiceSlugBatch_whenSvnRevisionRequested() {
        doReturn(Map.of(
                Path.of("ci/tms/src/main/java"),
                Optional.of(
                        AYaml.valid(Path.of("ci/a.yaml"), "ci", new StrongMode(false, Set.of()), false)
                )
        )).when(aYamlerService).getAbcServiceSlugBatch(any(), any());
        var response = grpcClient.getAbcServiceSlugBatch(
                Ayamler.GetAbcServiceSlugBatchRequest.newBuilder()
                        .addRequest(Ayamler.GetAbcServiceSlugRequest.newBuilder()
                                .setRevision("100")
                                .setPath("ci/tms/src/main/java"))
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetAbcServiceSlugBatchResponse.newBuilder()
                        .addAbcService(Ayamler.AbcService.newBuilder()
                                // check that controller doesn't return normalized svn revision "r100"
                                .setRevision("100")
                                .setPath("ci/tms/src/main/java")
                                .setAyaml(Ayamler.AYaml.newBuilder()
                                        .setPath("ci/a.yaml")
                                        .setService("ci")
                                        .setValid(true)
                                        .build()
                                )
                                .setSlug("ci")
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getAbcServiceSlugBatch_whenAYamlNotFound() {
        doReturn(Map.of(
                Path.of("ci/tms/src/main/java"),
                Optional.empty()
        )).when(aYamlerService).getAbcServiceSlugBatch(any(), any());
        var response = grpcClient.getAbcServiceSlugBatch(
                Ayamler.GetAbcServiceSlugBatchRequest.newBuilder()
                        .addRequest(Ayamler.GetAbcServiceSlugRequest.newBuilder()
                                .setRevision("a".repeat(40))
                                .setPath("ci/tms/src/main/java"))
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetAbcServiceSlugBatchResponse.newBuilder()
                        .addAbcService(Ayamler.AbcService.newBuilder()
                                .setRevision("a".repeat(40))
                                .setPath("ci/tms/src/main/java")
                                .setSlug("")
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getAbcServiceSlugBatch_whenManyPathsRequested() {
        doReturn(Map.of(
                Path.of("path1"), Optional.of(AYaml.valid(
                        Path.of("ci/a.yaml"), "ci", new StrongMode(false, Set.of()), false
                )),
                Path.of("path2"), Optional.of(AYaml.valid(
                        Path.of("ci/a.yaml"), "ci2", new StrongMode(false, Set.of()), false
                ))
        )).when(aYamlerService).getAbcServiceSlugBatch(
                eq(ArcRevision.of("a".repeat(40))),
                eq(Set.of(
                        Path.of("path1"),
                        Path.of("path2")
                ))
        );
        var response = grpcClient.getAbcServiceSlugBatch(
                Ayamler.GetAbcServiceSlugBatchRequest.newBuilder()
                        .addAllRequest(List.of(
                                Ayamler.GetAbcServiceSlugRequest.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path1")
                                        .build(),
                                Ayamler.GetAbcServiceSlugRequest.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path2")
                                        .build()
                        ))
                        .build()
        );

        assertThat(response).isEqualTo(
                Ayamler.GetAbcServiceSlugBatchResponse.newBuilder()
                        .addAllAbcService(List.of(
                                Ayamler.AbcService.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path1")
                                        .setAyaml(Ayamler.AYaml.newBuilder()
                                                .setPath("ci/a.yaml")
                                                .setValid(true)
                                                .setService("ci")
                                        )
                                        .setSlug("ci")
                                        .build(),
                                Ayamler.AbcService.newBuilder()
                                        .setRevision("a".repeat(40))
                                        .setPath("path2")
                                        .setAyaml(Ayamler.AYaml.newBuilder()
                                                .setPath("ci/a.yaml")
                                                .setValid(true)
                                                .setService("ci2")
                                        )
                                        .setSlug("ci2")
                                        .build()
                        ))
                        .build()
        );
    }

    @Override
    protected AYamlerServiceBlockingStub createStub(ManagedChannel channel) {
        return AYamlerServiceGrpc.newBlockingStub(channel);
    }

}
