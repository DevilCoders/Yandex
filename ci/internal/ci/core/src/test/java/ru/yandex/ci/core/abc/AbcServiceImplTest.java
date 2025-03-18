package ru.yandex.ci.core.abc;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.client.abc.AbcTestServer;
import ru.yandex.ci.core.spring.AbcConfig;
import ru.yandex.ci.core.spring.clients.AbcClientTestConfig;
import ru.yandex.ci.test.clock.OverridableClock;

@ContextConfiguration(classes = {
        AbcConfig.class,
        AbcClientTestConfig.class
})
class AbcServiceImplTest extends CommonYdbTestBase {

    @Autowired
    private AbcService abcService;

    @Autowired
    private AbcTestServer abcTestServer;

    @Autowired
    private OverridableClock clock;

    AbcServiceImpl abcServiceImpl;
    private String sendSingle;
    private List<String> sendList;
    private AbcServiceEntity expectSingle;
    private Map<String, AbcServiceEntity> expectList;

    @BeforeEach
    void init() {
        abcServiceImpl = (AbcServiceImpl) abcService;

        abcServiceImpl.flushCache();
        abcTestServer.reset();

        sendSingle = Abc.CI.getSlug();
        sendList = List.of(Abc.CI.getSlug(), Abc.INFRA.getSlug());

        clock.stop();
        var now = clock.instant();

        expectSingle = AbcServiceEntity.of(Abc.CI.toServiceInfo(), now);
        expectList = Map.of(
                Abc.CI.getSlug(), expectSingle,
                Abc.INFRA.getSlug(), AbcServiceEntity.of(Abc.INFRA.toServiceInfo(), now)
        );
    }

    @Test
    void testClearStub() {
        Assertions.assertThat(abcService.getService(sendSingle))
                .isEmpty();
        Assertions.assertThat(abcService.getServices(sendList))
                .isEmpty();
    }

    @Test
    void testCachedStub() {
        abcTestServer.addServices(Abc.CI.toServiceInfo(), Abc.INFRA.toServiceInfo());
        abcServiceImpl.syncServices();

        Assertions.assertThat(abcService.getService(sendSingle))
                .isEqualTo(Optional.of(expectSingle));

        Assertions.assertThat(abcService.getServices(sendList))
                .isEqualTo(expectList);

        abcTestServer.reset();
        abcServiceImpl.flushCache();

        // Loaded from YDB table
        Assertions.assertThat(abcService.getService(sendSingle))
                .isEqualTo(Optional.of(expectSingle));

        Assertions.assertThat(abcService.getServices(sendList))
                .isEqualTo(expectList);


        db.currentOrTx(() -> db.abcServices().deleteAll());

        // No data in database but stored in cache
        Assertions.assertThat(abcService.getService(sendSingle))
                .isEqualTo(Optional.of(expectSingle));

        Assertions.assertThat(abcService.getServices(sendList))
                .isEqualTo(expectList);


        abcServiceImpl.flushCache();
        // No data anywhere at last

        Assertions.assertThat(abcService.getService(sendSingle))
                .isEmpty();
        Assertions.assertThat(abcService.getServices(sendList))
                .isEmpty();
    }

}
