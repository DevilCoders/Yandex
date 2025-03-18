package ru.yandex.ci.ayamler.api.spring;

import java.util.concurrent.TimeUnit;

import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;

import ru.yandex.ci.ayamler.AYamlerService;
import ru.yandex.ci.common.application.profiles.CiProfile;

@Slf4j
@Configuration
@EnableScheduling
@Import({
        AYamlerServiceConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class AYamlerConfig {

    @Autowired
    private AYamlerService aYamlerService;

    @Scheduled(
            fixedRateString = "${ayamler.AYamlerConfig.refreshAbcCacheForKnownServicesSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void refreshAbcCacheForKnownServices() {
        log.info("refreshAbcCacheForKnownServices started");
        var stopwatch = Stopwatch.createStarted();
        try {
            aYamlerService.refreshAbcCacheForKnownServices();
        } finally {
            log.info("refreshAbcCacheForKnownServices finished in {}ms", stopwatch.elapsed(TimeUnit.MILLISECONDS));
        }
    }

}
