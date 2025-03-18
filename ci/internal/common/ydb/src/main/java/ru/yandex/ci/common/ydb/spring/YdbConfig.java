package ru.yandex.ci.common.ydb.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.ydb.YdbUtils;

@Configuration
public class YdbConfig {

    @Bean
    @Profile({CiProfile.NOT_UNIT_TEST_PROFILE})
    public KikimrConfig kikimrConfig(
            @Value("${ydb.kikimrConfig.ydbEndpoint}") String ydbEndpoint,
            @Value("${ydb.kikimrConfig.ydbDatabase}") String ydbDatabase,
            @Value("${ydb.kikimrConfig.ydbToken}") String ydbToken,
            @Value("${ydb.kikimrConfig.maxPoolSize}") int maxPoolSize
    ) {
        return YdbUtils.createConfig(ydbEndpoint, ydbDatabase, ydbToken, null)
                .withSessionPoolMax(maxPoolSize);
    }
}
