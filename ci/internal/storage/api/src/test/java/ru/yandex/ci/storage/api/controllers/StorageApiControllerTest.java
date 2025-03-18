package ru.yandex.ci.storage.api.controllers;

import java.time.Instant;
import java.util.Set;

import io.grpc.StatusRuntimeException;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.storage.api.ApiTestBase;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.api.check.CheckComparer;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGeneratorEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.large.AutocheckTasksFactory;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducerImpl;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

class StorageApiControllerTest extends ApiTestBase {
    private StorageApiServiceGrpc.StorageApiServiceBlockingStub stub;
    private BadgeEventsProducer badgeEventsProducer;
    private AutocheckTasksFactory autocheckTasksFactory;

    @BeforeEach
    public void setup() {
        // TODO: rewrite this, use Spring-based ApiCheckService and stuff
        var arcService = Mockito.mock(ArcService.class);
        for (var i = 1; i < 3; ++i) {
            when(arcService.getCommit(ArcRevision.of("r" + i)))
                    .thenReturn(ArcCommit.builder().id(ArcCommit.Id.of("r" + i)).svnRevision(i).build());
        }

        var requirementsService = mock(RequirementsService.class);
        badgeEventsProducer = mock(BadgeEventsProducer.class);
        autocheckTasksFactory = mock(AutocheckTasksFactory.class);
        var storageEventsProducer = mock(StorageEventsProducer.class);
        var storageProxyApi = mock(StorageProxyApiController.class);
        var storageApiController = new StorageApiController(
                new ApiCheckService(
                        new OverridableClock(),
                        requirementsService,
                        db,
                        arcService,
                        "test",
                        apiCache,
                        storageEventsProducer,
                        1,
                        mock(BazingaTaskManager.class),
                        ShardingSettings.DEFAULT
                ),
                apiCache,
                Mockito.mock(StorageEventsProducerImpl.class),
                badgeEventsProducer,
                autocheckTasksFactory,
                new CheckComparer(db),
                storageProxyApi
        );

        stub = StorageApiServiceGrpc.newBlockingStub(buildChannel(storageApiController));

        this.db.currentOrTx(() ->
                this.db.checkIds().save(new CheckIdGeneratorEntity(new CheckIdGeneratorEntity.Id(1), 1L))
        );
    }

    @Test
    public void createsCheck() {
        var request = StorageApi.RegisterCheckRequest.newBuilder()
                .setDiffSetId(1L)
                .setLeftRevision(
                        Common.OrderedRevision.newBuilder()
                                .setBranch("left-branch")
                                .setRevision("r1")
                                .setRevisionNumber(1)
                                .build()
                )
                .setRightRevision(
                        Common.OrderedRevision.newBuilder()
                                .setBranch("right-branch")
                                .setRevision("r2")
                                .setRevisionNumber(2)
                                .build()
                )
                .setOwner("author")
                .addTags("tag")
                .setTimestamp(ProtoConverter.convert(Instant.now()))
                .build();

        var response = stub.registerCheck(request);

        Assertions.assertEquals("author", response.getCheck().getOwner());
        var check = this.db.currentOrReadOnly(
                () -> this.db.checks().get(CheckEntity.Id.of(response.getCheck().getId()))
        );

        assertThat(check).isNotNull();
        assertThat(check.getAuthor()).isEqualTo("author");

        var responseTwo = stub.setTestenvId(
                StorageApi.SetTestenvIdRequest.newBuilder()
                        .setCheckId(check.getId().getId().toString())
                        .setTestenvId("te")
                        .build()
        );
        assertThat(responseTwo).isNotNull();
        check = this.db.currentOrReadOnly(
                () -> this.db.checks().get(CheckEntity.Id.of(response.getCheck().getId()))
        );

        assertThat(check.getTestenvId()).isEqualTo("te");

        var runningCheck = check.withStatus(CheckStatus.RUNNING);
        check = this.db.currentOrTx(() -> this.db.checks().save(runningCheck));
        var responseRetry = stub.registerCheck(request);
        assertThat(responseRetry.getCheck().getId()).isEqualTo(check.getId().toString());

        var checkCaptor = ArgumentCaptor.forClass(CheckEntity.class);
        verify(badgeEventsProducer).onCheckCreated(checkCaptor.capture());
        assertThat(checkCaptor.getValue().getId()).isEqualTo(check.getId());
        assertThat(checkCaptor.getValue().getStatus()).isEqualTo(CheckStatus.CREATED);

    }

    @Test
    public void findsByCheckIds() {
        this.db.currentOrTx(() -> this.db.checks().save(
                CheckEntity.builder()
                        .id(CheckEntity.Id.of(1L))
                        .left(new StorageRevision("trunk", "r1", 0, Instant.EPOCH))
                        .right(new StorageRevision("trunk", "r2", 0, Instant.EPOCH))
                        .diffSetId(1L)
                        .status(CheckStatus.RUNNING)
                        .author("author")
                        .created(Instant.now())
                        .tags(Set.of("tag"))
                        .type(CheckType.TRUNK_POST_COMMIT)
                        .build()
                )
        );

        var request = StorageApi.FindCheckByRevisionsRequest.newBuilder()
                .setLeftRevision("r1")
                .setRightRevision("r2")
                .addTags("tag")
                .build();
        var response = stub.findChecksByRevisions(request);

        Assertions.assertEquals(1, response.getChecksList().size());
        Assertions.assertEquals("1", response.getChecks(0).getId());
    }

    @Test
    public void cancelsCheck() {
        var check = CheckEntity.builder()
                .id(CheckEntity.Id.of(1L))
                .left(StorageRevision.EMPTY)
                .right(StorageRevision.EMPTY)
                .diffSetId(1L)
                .status(CheckStatus.RUNNING)
                .author("author")
                .created(Instant.now())
                .tags(Set.of("tag"))
                .type(CheckType.TRUNK_POST_COMMIT)
                .build();

        var iteration = CheckIterationEntity.builder()
                .id(CheckIterationEntity.Id.of(CheckEntity.Id.of(1L), IterationType.FULL, 1))
                .status(CheckStatus.CREATED)
                .created(Instant.now())
                .build();

        this.db.currentOrTx(
                () -> {
                    this.db.checks().save(check);
                    this.db.checkIterations().save(iteration);
                }
        );

        var request = StorageApi.CancelCheckRequest.newBuilder()
                .setId("1")
                .build();

        var response = stub.cancelCheck(request);
        assertThat(response).isNotNull();

        var updatedCheck = this.db.currentOrReadOnly(
                () -> this.db.checks().get(check.getId())
        );

        var updatedIteration = this.db.currentOrReadOnly(
                () -> this.db.checkIterations().get(iteration.getId())
        );

        assertThat(updatedCheck.getStatus()).isEqualTo(CheckStatus.CANCELLING);
        assertThat(updatedIteration.getStatus()).isEqualTo(CheckStatus.CANCELLING);

        //noinspection ResultOfMethodCallIgnored
        assertThatThrownBy(() -> stub.cancelCheck(
                StorageApi.CancelCheckRequest.newBuilder()
                        .setId("bad")
                        .build()
        )).isInstanceOf(StatusRuntimeException.class)
                .hasMessage("INVALID_ARGUMENT: Unable to parse check id as number: bad");
    }
}
