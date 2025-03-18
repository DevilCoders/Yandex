package ru.yandex.ci;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.TestExecutionListeners;
import org.springframework.test.context.TestExecutionListeners.MergeMode;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.ydb.CommonYdbTestConfig;
import ru.yandex.ci.test.YdbCleanupTestListener;

@ContextConfiguration(classes = {
        CommonYdbTestConfig.class,
})
@TestExecutionListeners(
        value = {YdbCleanupTestListener.class},
        mergeMode = MergeMode.MERGE_WITH_DEFAULTS)
public class CommonYdbTestBase extends CommonTestBase {

    @Autowired
    protected CiMainDb db;
}
