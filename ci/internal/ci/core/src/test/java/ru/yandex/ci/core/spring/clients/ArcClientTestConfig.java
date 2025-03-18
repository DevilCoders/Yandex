package ru.yandex.ci.core.spring.clients;

import lombok.extern.slf4j.Slf4j;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceCanonImpl;
import ru.yandex.ci.core.arc.ArcServiceStub;

@Slf4j
@Configuration
public class ArcClientTestConfig {
    public static final String CANON_ARC_PROFILE = "arc-canon";
    private static final boolean DO_CANONIZE_ARC_CALLS = false;

    @Bean
    @Profile("!" + CANON_ARC_PROFILE)
    public ArcService arcService() {
        log.info("Creating ArcServiceStub");
        return new ArcServiceStub();
    }

    @Bean
    @Profile(CANON_ARC_PROFILE)
    public ArcService arcServiceCanon() {
        log.info("Creating ArcServiceCanonImpl");
        return new ArcServiceCanonImpl(DO_CANONIZE_ARC_CALLS);
    }
}
