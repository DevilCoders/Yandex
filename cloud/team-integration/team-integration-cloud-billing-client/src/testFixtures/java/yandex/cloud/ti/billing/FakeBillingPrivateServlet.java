package yandex.cloud.ti.billing;

import java.io.IOException;
import java.io.Serial;

import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

@WebServlet(name = "fake-billing-private-servlet", urlPatterns = "/billing/v1/private/*")
public class FakeBillingPrivateServlet extends HttpServlet {

    @Serial
    private static final long serialVersionUID = -6605007936007574823L;

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws IOException {
        String path = req.getServletPath() + req.getPathInfo();
        var stream = getClass().getResourceAsStream(path);
        if (stream == null) {
            resp.sendError(HttpServletResponse.SC_NOT_FOUND, path + " not found");
        } else {
            try (var outputStream = resp.getOutputStream()) {
                stream.transferTo(outputStream);
            }
        }
    }

    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp) throws IOException {
        String path = req.getServletPath() + req.getPathInfo();
        var stream = getClass().getResourceAsStream(path);
        if (stream == null) {
            resp.sendError(HttpServletResponse.SC_NOT_FOUND, path + " not found");
        } else {
            try (var outputStream = resp.getOutputStream()) {
                stream.transferTo(outputStream);
            }
        }
    }

}
