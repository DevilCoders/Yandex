package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcBranchCache;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.spring.clients.ArcClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(ArcClientConfig.class)
public class TestBranchCache extends AbstractSpringBasedApp {

    @Autowired
    ArcService arcService;

    @Override
    protected void run() {
        this.testNewCache();
    }

    private void testNewCache() {
        // Loaded in 39 msec
        var cache = new ArcBranchCache(arcService);
        cache.updateCache();

        var branch = "users/svemarch/TM-3519";
        var branches = cache.suggestBranches(branch).limit(20).toList();
        log.info("Suggest: {}", branches);
        for (int i = 0; i < 20; i++) {
            var now = System.currentTimeMillis();
            branches = cache.suggestBranches(branch).limit(20).toList();
            log.info("Loaded in {} msec: {} items", System.currentTimeMillis() - now, branches.size());
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
