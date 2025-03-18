package ru.yandex.ci.common.application;

import lombok.extern.slf4j.Slf4j;
import org.springframework.boot.builder.SpringApplicationBuilder;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.core.env.AbstractEnvironment;

import ru.yandex.ci.common.application.profiles.CiProfile;

@Slf4j
public class CiApplication {

    // Use in Properties config
    private static final String CI_APP_NAME_PROPERTY = "app.name";
    private static final String CI_APP_TYPE_PROPERTY = "app.type";

    // TODO: remove after throwing out 'iceberg'
    private static final String YANDEX_ENVIRONMENT_TYPE_PROPERTY = "yandex.environment.type";

    private CiApplication() {
    }

    public static void init(String appType, String appName, ConfigurableApplicationContext applicationContext) {
        System.setProperty(CI_APP_TYPE_PROPERTY, appType);
        System.setProperty(CI_APP_NAME_PROPERTY, appName);

        String profile = System.getProperty(AbstractEnvironment.ACTIVE_PROFILES_PROPERTY_NAME);
        if (profile == null) {
            profile = CiProfile.LOCAL_PROFILE;
            applicationContext.getEnvironment().setActiveProfiles(profile);
            System.setProperty(AbstractEnvironment.ACTIVE_PROFILES_PROPERTY_NAME, profile);
        }

        String envType = "stable".equals(profile) ? "production" : profile;
        System.setProperty(
                YANDEX_ENVIRONMENT_TYPE_PROPERTY,
                envType
        );

        log.info("CI Application [{}/{}] configured, profile: {}, environment type: {}, additional profile: {}",
                appType, appName, profile, envType, System.getProperty("app.secrets.profile", ""));
    }

    public static void run(String[] args, Class<?> mainClass, String appType, String appName) {
        new SpringApplicationBuilder(mainClass)
                .initializers(new CiApplicationInitializer(appType, appName))
                .sources(PropertiesConfig.class, ConversionConfig.class)
                .run(args);
    }

    public static String getApplicationEnvironment() {
        return System.getProperty(AbstractEnvironment.ACTIVE_PROFILES_PROPERTY_NAME, CiProfile.LOCAL_PROFILE);
    }
}
