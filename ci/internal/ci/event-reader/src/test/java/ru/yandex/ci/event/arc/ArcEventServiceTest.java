package ru.yandex.ci.event.arc;

import java.util.stream.Stream;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.arc.api.Message;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreStubConfig;
import ru.yandex.ci.common.grpc.ProtobufTestUtils;
import ru.yandex.ci.engine.discovery.arc_reflog.ProcessArcReflogRecordTask;
import ru.yandex.ci.event.spring.ArcServiceConfig;
import ru.yandex.ci.event.spring.CiEventReaderPropertiesConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.test.BazingaTaskManagerStub;

import static org.assertj.core.api.Assertions.assertThat;

@Slf4j
@ContextConfiguration(classes = {
        ArcServiceConfig.class,
        BazingaCoreStubConfig.class,
        CiEventReaderPropertiesConfig.class
})
class ArcEventServiceTest extends YdbCiTestBase {

    @Autowired
    private BazingaTaskManager bazingaTaskManager;

    @Autowired
    private ArcEventService arcEventService;

    private BazingaTaskManagerStub bazingaTaskManagerStub;

    @BeforeEach
    void setUp() {
        bazingaTaskManagerStub = (BazingaTaskManagerStub) bazingaTaskManager;
    }

    @AfterEach
    protected void resetEngineTest() {
        bazingaTaskManagerStub.clearTasks();
    }

    @ParameterizedTest(name = "{1}")
    @MethodSource("reflog")
    void registerTask(Message.ReflogRecord reflogRecord, String message) {
        log.info("Processing {}", message);
        arcEventService.processEvent(reflogRecord);

        assertThat(bazingaTaskManagerStub.getJobsParameters(ProcessArcReflogRecordTask.class))
                .hasSize(1)
                .first()
                .isEqualTo(
                        new ProcessArcReflogRecordTask.ReflogRecord(
                                reflogRecord.getName(), reflogRecord.getAfterOid(), reflogRecord.getBeforeOid()
                        )
                );
    }

    @ParameterizedTest(name = "{1}")
    @MethodSource("reflog")
    void skipUserBranches(Message.ReflogRecord reflogRecord, String message) {
        log.info("Processing {}", message);
        arcEventService.processEvent(reflogRecord.toBuilder()
                .setName("user/rembo/uber-fix-CI-9000")
                .build());
        assertThat(bazingaTaskManagerStub.getJobsParameters(ProcessArcReflogRecordTask.class)).isEmpty();
    }


    static Stream<Arguments> reflog() {
        return Stream.of(
                "arc/merge-review-to-release-branch",
                "arc/create-release-branch-on-trunk",
                "arc/fast-forward-release-branch"
        ).map(file -> Arguments.of(ArcEventServiceTest.readProtobuf(file), file));
    }

    private static Message.ReflogRecord readProtobuf(String source) {
        return ProtobufTestUtils.parseProtoText(source, Message.ReflogRecord.class);
    }
}
