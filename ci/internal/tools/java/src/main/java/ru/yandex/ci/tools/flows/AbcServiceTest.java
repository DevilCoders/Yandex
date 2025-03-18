package ru.yandex.ci.tools.flows;

import java.util.Set;

import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.abc.AbcFavoriteProjectsService;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.spring.AbcConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import({
        YdbCiConfig.class,
        AbcConfig.class
})
@Configuration
public class AbcServiceTest extends AbstractSpringBasedApp {

    private final String username = "miroslav2";
    private final String abcSlug = "ci";

    @Autowired
    AbcService abcService;

    @Autowired
    AbcFavoriteProjectsService abcFavoriteProjectsService;

    @Override
    protected void run() {
        abcServiceMapper();
    }

    void abcService() {
        checkScope("administration", true);
        checkScope("Administration", true);
        checkScope("Администрирование", true);
        checkScope("8", true);

        checkScope("administration1", false);
        checkScope("Administration1", false);
        checkScope("Администрирование1", false);
        checkScope("81", false);

        //

        checkRole("other", true);
        checkRole("System administrator", true);
        checkRole("Системный администратор", true);
        checkRole("16", true);

        checkRole("administration", false);
        checkRole("Administration", false);
        checkRole("Администрирование", false);
        checkRole("81", false);

        //

        //checkDuty("task-duty", true);
        //checkDuty("CI Task Duty", true);
        //checkDuty("3080", true);

        checkDuty("task-duty1", false);
        checkDuty("CI Task Duty1", false);
        checkDuty("30801", false);

        log.info("has access: {}", abcService.isMember("dmfedin", "maps-core-mobile-arcadia-ci"));
    }

    void abcServiceMapper() {
        // TODO: careful
        // abcFavoriteProjectsService.syncFavoriteProjects();
    }

    private void checkScope(String scope, boolean expectAccess) {
        var hasAccess = abcService.isMember(username, abcSlug, Set.of(scope));
        log.info("User [{}], abc [{}], scope [{}], access expect: {}, actual: {}",
                username, abcSlug, scope, expectAccess, hasAccess);
        Preconditions.checkState(expectAccess == hasAccess);
    }

    private void checkRole(String role, boolean expectAccess) {
        var hasAccess = abcService.isMember(username, abcSlug, Set.of(), Set.of(role), Set.of());
        log.info("User [{}], abc [{}], role [{}], access expect: {}, actual: {}",
                username, abcSlug, role, expectAccess, hasAccess);
        Preconditions.checkState(expectAccess == hasAccess);
    }

    private void checkDuty(String duty, boolean expectAccess) {
        var hasAccess = abcService.isMember(username, abcSlug, Set.of(), Set.of(), Set.of(duty));
        log.info("User [{}], abc [{}], duty [{}], access expect: {}, actual: {}",
                username, abcSlug, duty, expectAccess, hasAccess);
        Preconditions.checkState(expectAccess == hasAccess);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

}
