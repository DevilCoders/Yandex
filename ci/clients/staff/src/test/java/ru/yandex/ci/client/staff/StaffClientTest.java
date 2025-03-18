package ru.yandex.ci.client.staff;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmHeaders;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.passport.tvmauth.TvmClient;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.when;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
@ExtendWith(MockitoExtension.class)
class StaffClientTest {

    private static final int YAV_TVM_ID = 42;
    private static final String TVM_TICKET = "secret";

    private static final String CI_USER = "andreevdm";
    private static final String NOT_CI_USER = "antipov93";
    private static final String OUT_STAFF_USER = "daria-lu";
    private static final String ROBOT_USER = "robot-ci";


    private final MockServerClient server;
    private StaffClient client;

    @Mock
    private TvmClient tvmClient;


    StaffClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        var properties = HttpClientProperties.builder()
                .endpoint("http:/" + server.remoteAddress() + "/")
                .authProvider(new TvmAuthProvider(tvmClient, YAV_TVM_ID))
                .build();
        client = StaffClient.create(properties);
        when(tvmClient.getServiceTicketFor(YAV_TVM_ID)).thenReturn(TVM_TICKET);

    }

    @Test
    void isUserInAbcService() {
        String abcCi = "ci";
        String developmentRole = "development";
        String notExistingRole = "other_role";
        String abcCiDevelopment = abcCi + "_" + developmentRole;
        String abcCiNotExisting = abcCi + "_" + notExistingRole;

        mockMembership(abcCi, CI_USER, true);
        mockMembership(abcCi, NOT_CI_USER, false);

        mockMembership(abcCiDevelopment, CI_USER, true);
        mockMembership(abcCiNotExisting, CI_USER, false);
        mockMembership(abcCiDevelopment, NOT_CI_USER, false);

        assertThat(client.isUserInAbcService(CI_USER, abcCi)).isTrue();
        assertThat(client.isUserInAbcService(NOT_CI_USER, abcCi)).isFalse();

        assertThat(client.isUserInAbcService(CI_USER, abcCi, developmentRole)).isTrue();
        assertThat(client.isUserInAbcService(CI_USER, abcCi, notExistingRole)).isFalse();
        assertThat(client.isUserInAbcService(NOT_CI_USER, abcCi, developmentRole)).isFalse();

    }

    @Test
    void outStaff() {
        mockPerson(CI_USER);
        mockPerson(OUT_STAFF_USER);
        mockPerson(ROBOT_USER);

        assertThat(client.getStaffPerson(CI_USER).getOfficial().isOutStaff()).isFalse();
        assertThat(client.getStaffPerson(OUT_STAFF_USER).getOfficial().isOutStaff()).isTrue();
        assertThat(client.getStaffPerson(ROBOT_USER).getOfficial().isOutStaff()).isFalse();
    }

    private void mockMembership(String group, String login, boolean existing) {
        String resourceFile = "groupmembership/";
        if (existing) {
            resourceFile += StaffClient.ABC_SERVICE_GROUP_PREFIX + group + "__" + login + ".json";
        } else {
            resourceFile += "_not_member.json";
        }
        server.when(request("/v3/groupmembership")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("group.url", StaffClient.ABC_SERVICE_GROUP_PREFIX + group)
                        .withQueryStringParameter("person.login", login)
                        .withHeader(TvmHeaders.SERVICE_TICKET, TVM_TICKET)
                )
                .respond(response(resource(resourceFile)));
    }

    private void mockPerson(String login) {
        server.when(request("/v3/persons")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("login", login)
                        .withQueryStringParameter("_one", "1")
                        .withHeader(TvmHeaders.SERVICE_TICKET, TVM_TICKET)
                )
                .respond(response(resource("persons/one/" + login + ".json")));
    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }
}
