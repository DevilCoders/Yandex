package ru.yandex.ci;

import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.boot.test.mock.mockito.ResetAndCleanupListener;
import org.springframework.test.context.ActiveProfiles;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.TestExecutionListeners;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.test.LoggingTestListener;

@ExtendWith(SpringExtension.class)
@ContextConfiguration(classes = CommonTestConfig.class)
@ActiveProfiles(profiles = CiProfile.UNIT_TEST_PROFILE)
@TestExecutionListeners(
        value = {LoggingTestListener.class, ResetAndCleanupListener.class},
        mergeMode = TestExecutionListeners.MergeMode.MERGE_WITH_DEFAULTS)
public class CommonTestBase {
}
