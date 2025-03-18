package ru.yandex.ci.ayamler.api.spring;

import java.time.Duration;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.ayamler.AYamlerService;
import ru.yandex.ci.ayamler.AYamlerServiceProperties;
import ru.yandex.ci.ayamler.AbcService;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AYamlService;

@Configuration
@Import({
        ArcConfig.class,
        AbcServiceConfig.class
})
public class AYamlerServiceConfig {

    @Bean
    public AYamlerServiceProperties aYamlerServiceProperties(
            @Value("${ayamler.aYamlerServiceProperties.expireAfterAccess}") Duration expireAfterAccess,
            @Value("${ayamler.aYamlerServiceProperties.maximumSize}") int maximumSize,
            @Value("${ayamler.aYamlerServiceProperties.oidExpireAfterAccess}") Duration oidExpireAfterAccess,
            @Value("${ayamler.aYamlerServiceProperties.oidMaximumSize}") int oidMaximumSize,
            @Value("${ayamler.aYamlerServiceProperties.strongExpireAfterAccess}") Duration strongExpireAfterAccess,
            @Value("${ayamler.aYamlerServiceProperties.strongMaximumSize}") int strongMaximumSize,
            @Value("${ayamler.aYamlerServiceProperties.cacheConcurrencyLevel}") int cacheConcurrencyLevel
    ) {
        return AYamlerServiceProperties.builder()
                .aYamlCacheExpireAfterAccess(expireAfterAccess)
                .aYamlCacheMaximumSize(maximumSize)
                .aYamlOidCacheExpireAfterAccess(oidExpireAfterAccess)
                .aYamlOidCacheMaximumSize(oidMaximumSize)
                .strongModeCacheExpireAfterAccess(strongExpireAfterAccess)
                .strongModeCacheMaximumSize(strongMaximumSize)
                .cacheConcurrencyLevel(cacheConcurrencyLevel)
                .build();
    }

    @Bean
    public AYamlerService aYamlerService(
            ArcService arcService,
            AYamlService aYamlService,
            AbcService abcService,
            MeterRegistry meterRegistry,
            AYamlerServiceProperties aYamlerServiceProperties
    ) {
        return new AYamlerService(arcService, aYamlService, abcService, meterRegistry, aYamlerServiceProperties);
    }

    @Bean
    public AYamlService aYamlService(
            ArcService arcService,
            @Nullable MeterRegistry meterRegistry,
            @Value("${ayamler.aYamlService.configCacheExpireAfterAccess}") Duration configCacheExpireAfterAccess,
            @Value("${ayamler.aYamlService.configCacheMaximumSize}") int configCacheMaximumSize
    ) {
        return new AYamlService(
                arcService,
                meterRegistry,
                configCacheExpireAfterAccess,
                configCacheMaximumSize
        );
    }
}
