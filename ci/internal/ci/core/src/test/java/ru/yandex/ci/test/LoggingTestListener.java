package ru.yandex.ci.test;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;
import org.springframework.core.Ordered;
import org.springframework.core.annotation.Order;
import org.springframework.test.context.TestContext;
import org.springframework.test.context.TestExecutionListener;

@Slf4j
@Order(Ordered.HIGHEST_PRECEDENCE)
public class LoggingTestListener implements TestExecutionListener {
    private long onTestClass;
    private long onTestMethod;

    @Override
    public void beforeTestClass(TestContext testContext) {
        log.info(" >>> Begin Test Class [{}] ===", testContext.getTestClass());
        onTestClass = System.currentTimeMillis();
    }

    @Override
    public void beforeTestMethod(TestContext testContext) {
        log.info("  >> Begin Test Method [{}] ==", testContext.getTestMethod());
        onTestMethod = System.currentTimeMillis();
    }

    @Override
    public void afterTestMethod(TestContext testContext) {
        long diff = System.currentTimeMillis() - onTestMethod;
        @Nullable var t = testContext.getTestException();
        log.info("  << End Test Method in {} msec [{}] [{}] ==", diff, testContext.getTestMethod(), t);
    }

    @Override
    public void afterTestClass(TestContext testContext) {
        long diff = System.currentTimeMillis() - onTestClass;
        log.info(" <<< End Test Class in {} msec [{}] ===", diff, testContext.getTestClass());
    }
}
