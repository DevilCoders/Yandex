package yandex.cloud.team.integration.idm;

import java.util.Set;

import javax.ws.rs.core.Application;

import yandex.cloud.team.integration.idm.http.IdmServiceExceptionMapper;
import yandex.cloud.team.integration.idm.http.servlet.Idm;

/**
 * Registered in IdmServiceHttpServletDispatcher
 */
public class IdmServiceRestApplication extends Application {

    @Override
    public Set<Object> getSingletons() {
        return Set.of(
            new IdmServiceExceptionMapper(),
            new Idm()
        );
    }

}
