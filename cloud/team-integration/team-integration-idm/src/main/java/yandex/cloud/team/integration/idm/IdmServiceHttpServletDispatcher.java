package yandex.cloud.team.integration.idm;

import javax.servlet.annotation.HttpConstraint;
import javax.servlet.annotation.ServletSecurity;
import javax.servlet.annotation.WebInitParam;
import javax.servlet.annotation.WebServlet;

import org.jboss.resteasy.plugins.server.servlet.HttpServletDispatcher;

/**
 * Internal class to map the URL patterns to the corresponding REST application.
 */
@WebServlet(
    name = "resteasy",
    urlPatterns = {"/idm/*"},
    initParams = {
        @WebInitParam(name = "javax.ws.rs.core.Application",
            value = "yandex.cloud.team.integration.idm.IdmServiceRestApplication")
    })
@ServletSecurity(@HttpConstraint(rolesAllowed = {"TVM_USER"}))
public class IdmServiceHttpServletDispatcher extends HttpServletDispatcher {

        // This class contains no code

}
