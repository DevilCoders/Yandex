package ru.yandex.ci.tools;

import java.util.stream.Stream;

import com.beust.jcommander.JCommander;
import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;
import org.slf4j.Logger;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.web.context.support.AnnotationConfigWebApplicationContext;

import ru.yandex.ci.common.application.CiApplication;
import ru.yandex.ci.common.application.ConversionConfig;
import ru.yandex.ci.common.application.PropertiesConfig;
import ru.yandex.ci.core.spring.CommonConfig;

@Slf4j
@Configuration
@Import({
        ConversionConfig.class,
        CommonConfig.class
})
public abstract class AbstractSpringBasedApp {

    private static final String DEFAULT_APP_TYPE = "ci";
    private static final String DEFAULT_APP_NAME = "ci-tms";

    static {
        System.setProperty("ydb.skip.checks", "true");
        System.setProperty("ci.sourceCodeEntityService.resolveBeansOnRefresh", "false");
    }

    //

    protected abstract void run() throws Exception;

    private static Logger getLogger(Class<?> topLevelClass) throws IllegalAccessException {
        var loggerFieldOptional = Stream.of(topLevelClass.getDeclaredFields())
                .filter(f -> f.getType() == Logger.class)
                .findFirst();
        if (loggerFieldOptional.isPresent()) {
            var loggerField = loggerFieldOptional.get();
            loggerField.setAccessible(true);
            return (Logger) loggerField.get(null);
        } else {
            return log;
        }
    }

    protected static <T> void startAndStopThisClass(String[] args) {
        startAndStopThisClass(args, DEFAULT_APP_TYPE, DEFAULT_APP_NAME, Environment.TESTING);
    }

    protected static <T> void startAndStopThisClass(String[] args, Environment env) {
        startAndStopThisClass(args, DEFAULT_APP_TYPE, DEFAULT_APP_NAME, env);
    }

    protected static <T> void startAndStopThisClass(String[] args, String appType, String appName) {
        startAndStopThisClass(args, appType, appName, Environment.TESTING);
    }

    protected static <T> void startAndStopThisClass(String[] args, String appType, String appName, Environment env) {
        try {
            var stackTrace = Thread.currentThread().getStackTrace();
            var topLevelClassName = stackTrace[stackTrace.length - 1].getClassName();
            Class<?> topLevelClass = AbstractSpringBasedApp.class.getClassLoader().loadClass(topLevelClassName);
            var logger = getLogger(topLevelClass);

            Preconditions.checkState(AbstractSpringBasedApp.class.isAssignableFrom(topLevelClass),
                    "class %s should extend %s", AbstractSpringBasedApp.class);

            @SuppressWarnings("unchecked")
            var runnableClass = (Class<AbstractSpringBasedApp>) topLevelClass;
            startAndStop(logger, runnableClass, AbstractSpringBasedApp::run, args, appType, appName, env);

        } catch (ClassNotFoundException e) {
            System.err.println("Cannot find class");
            e.printStackTrace();
            System.exit(1);
        } catch (IllegalAccessException e) {
            System.err.println("Cannot get logger field");
            e.printStackTrace();
            System.exit(1);
        } catch (Throwable throwable) {
            System.err.println("Unknown error");
            throwable.printStackTrace();
            System.exit(1);
        }
    }

    private static <T> void startAndStop(
            Logger logger,
            Class<T> clazz,
            BeanCall<T> call,
            String[] args,
            String appType,
            String appName,
            Environment env
    ) {
        System.setProperty("app.secrets.profile", env.name().toLowerCase());

        try (var ctx = new AnnotationConfigWebApplicationContext()) {
            CiApplication.init(appType, appName, ctx);

            switch (env) {
                case STABLE -> ctx.register(PropertiesConfig.class, PropertySourceStable.class);
                case TESTING -> ctx.register(PropertiesConfig.class);
            }

            ctx.register(clazz);
            ctx.refresh();

            T bean = ctx.getBean(clazz);

            JCommander jCommander = new JCommander(bean);
            jCommander.parse(args);

            call.call(bean);

            logger.info("Execution complete");

            ctx.close();
            System.exit(0);
        } catch (Throwable e) {
            logger.error("Unexpected exception during application startup", e);
            System.exit(1);
        }
    }

    protected enum Environment {
        STABLE, TESTING
    }

    protected interface BeanCall<T> {
        void call(T bean) throws Throwable;
    }

}
