package ru.yandex.ci.test;

import javax.annotation.Nonnull;

import org.springframework.test.context.TestContext;
import org.springframework.test.context.TestExecutionListener;

import ru.yandex.ci.ydb.YdbCleanupReset;

public class YdbCleanupTestListener implements TestExecutionListener {

    @Override
    public void beforeTestMethod(@Nonnull TestContext testContext) {
        var context = testContext.getApplicationContext();
        do {
            context.getBeansOfType(YdbCleanupReset.class).values().forEach(YdbCleanupReset::reset);
            context = context.getParent();
        } while (context != null);

    }
}
