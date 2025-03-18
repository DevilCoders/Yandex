package ru.yandex.ci.engine.pcm;

import java.nio.file.Path;
import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.db.autocheck.model.AccessControl;

class PCMSelectorTest extends CommonTestBase {
    private static final String MOCK_USER = "mockUser";

    @MockBean
    PCMService pcmService;

    @MockBean
    AbcService abcService;

    PCMSelector selector;

    @BeforeEach
    void setUp() {
        Mockito.when(abcService.getServicesMembers(MOCK_USER)).thenReturn(List.of());
        Mockito.when(pcmService.getAvailablePools(Mockito.anyList())).thenReturn(List.of(
                new PoolNode(AccessControl.ofUser(MOCK_USER), "pool1", 1, 10),
                new PoolNode(AccessControl.ofUser(MOCK_USER), "pool2", 2, 10),
                new PoolNode(AccessControl.ALL_STAFF, "pool3", 2, 8)
        ));

        selector = new PCMSelector(abcService, pcmService);
    }

    @Test
    void selectPoolPessimizedPath() {
        Assertions.assertEquals(
                PCMSelector.PRECOMMITS_COMMON_COMP_PATH_POOL,
                selector.selectPool(Set.of(Path.of("build/scripts/test")), MOCK_USER)
        );
        Mockito.verifyNoInteractions(abcService, pcmService);
    }

    @Test
    void selectPoolMaxAvailable() {
        Assertions.assertEquals(
                "pool2",
                selector.selectPool(Set.of(Path.of("ci")), MOCK_USER)
        );
        Mockito.verify(pcmService).getAvailablePools(List.of(AccessControl.ofUser(MOCK_USER), AccessControl.ALL_STAFF));
    }

    @Test
    void selectPoolCommonComponentsByPath() {
        Assertions.assertEquals(
                PCMSelector.PRECOMMITS_COMMON_COMP_PATH_POOL,
                selector.selectPool(Set.of(Path.of("contrib/libs/cxxsupp/system_stl")), MOCK_USER)
        );
        Mockito.verifyNoInteractions(abcService, pcmService);
    }

    @Test
    void selectPoolNoAvailablePools() {
        Mockito.when(pcmService.getAvailablePools(Mockito.anyList())).thenReturn(List.of());

        Assertions.assertEquals(
                PCMSelector.PRECOMMITS_DEFAULT_POOL,
                selector.selectPool(Set.of(Path.of("ci")), MOCK_USER)
        );
        Mockito.verify(pcmService).getAvailablePools(List.of(AccessControl.ofUser(MOCK_USER), AccessControl.ALL_STAFF));
    }
}
