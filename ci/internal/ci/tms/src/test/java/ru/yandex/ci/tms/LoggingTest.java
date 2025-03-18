package ru.yandex.ci.tms;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.core.Appender;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.appender.AbstractAppender;
import org.apache.logging.log4j.core.config.Configuration;
import org.apache.logging.log4j.core.config.LoggerConfig;
import org.apache.logging.log4j.core.layout.PatternLayout;
import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

public class LoggingTest {

    private static final String LOG4J = "log4j";
    private static final String LOG4J2 = "log4j2";
    private static final String SLF4J = "slf4j";
    private static final String JCL = "jcl";
    private static final String LOGGER_NAME = "test";
    private static final String APPENDER_NAME = "test-appender";

    private static String message;

    @BeforeAll
    static void init() {
        LoggerContext ctx = (LoggerContext) LogManager.getContext(false);
        Configuration config = ctx.getConfiguration();

        // create appender
        PatternLayout layout = PatternLayout.newBuilder().withPattern("%m").build();
        Appender appender = new AbstractAppender(APPENDER_NAME, null, layout, true, null) {
            @Override
            public void append(LogEvent event) {
                message = event.getMessage().getFormattedMessage();
            }
        };
        appender.start();

        // create logger config for writing to the appender
        // additive = true allows test messages to be seen in the console
        LoggerConfig loggerConfig = new LoggerConfig(LOGGER_NAME, Level.ALL, true);
        loggerConfig.addAppender(appender, Level.ALL, null);
        config.addLogger(LOGGER_NAME, loggerConfig);

        // important for non-log4j2 loggers
        ctx.updateLoggers();

        LogManager.getLogger(LoggingTest.class).debug("Console appender is working");
    }

    @AfterAll
    static void cleanup() {
        message = null;
        LoggerContext ctx = (LoggerContext) LogManager.getContext(false);
        // reset to log4j2.xml
        ctx.reconfigure();
    }

    @Test
    void log4j2() {
        org.apache.logging.log4j.Logger logger = org.apache.logging.log4j.LogManager.getLogger(LOGGER_NAME);
        logger.debug(LOG4J2);
        Assertions.assertEquals(LOG4J2, message);
    }

    @Test
    void log4j() {
        org.apache.log4j.Logger logger = org.apache.log4j.Logger.getLogger(LOGGER_NAME);
        logger.debug(LOG4J);
        Assertions.assertEquals(LOG4J, message);
    }

    @Test
    void slf4j() {
        org.slf4j.Logger logger = org.slf4j.LoggerFactory.getLogger(LOGGER_NAME);
        logger.debug(SLF4J);
        Assertions.assertEquals(SLF4J, message);
    }

    @Test
    public void jcl() {
        org.apache.commons.logging.Log logger = org.apache.commons.logging.LogFactory.getLog(LOGGER_NAME);
        logger.debug(JCL);
        Assertions.assertEquals(JCL, message);
    }

}
