package ru.yandex.ci.client.blackbox;

import java.util.Set;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.blackbox.OAuthResponse.OAuthInfo;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
class BlackboxClientImplTest {

    private final MockServerClient server;
    private BlackboxClientImpl blackboxClient;

    BlackboxClientImplTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        blackboxClient = BlackboxClientImpl.create(HttpClientPropertiesStub.of(server));
    }

    @Test
    void getUserInfo() {
        server.when(request("/blackbox")
                .withQueryStringParameter("method", "userinfo")
                .withQueryStringParameter("format", "json")
                .withQueryStringParameter("userip", "127.0.0.1")
                .withQueryStringParameter("uid", "12345"))
                .respond(response().withBody("""
                        {
                          "users": [
                            {
                              "id": "1120000000141708",
                              "uid": {
                                "value": "1120000000141708",
                                "lite": false,
                                "hosted": false
                              },
                              "login": "miroslav2",
                              "have_password": true,
                              "have_hint": false,
                              "karma": {
                                "value": 0
                              },
                              "karma_status": {
                                "value": 0
                              }
                            }
                          ]
                        }
                        """));

        assertEquals(
                new UserInfoResponse("miroslav2"),
                blackboxClient.getUserInfo("127.0.0.1", 12345)
        );
    }

    @Test
    void getOAuth() {
        server.when(request("/blackbox")
                .withQueryStringParameter("method", "oauth")
                .withQueryStringParameter("format", "json")
                .withQueryStringParameter("userip", "127.0.0.1")
                .withQueryStringParameter("oauth_token", "token"))
                .respond(response().withBody("""
                        {
                          "oauth": {
                            "uid": "1120000000208259",
                            "token_id": "1040137",
                            "device_id": "",
                            "device_name": "",
                            "scope": "oauth:grant_xtoken sandbox:api staff:read startrek:write arcanum:api_read \
                            vault:use",
                            "ctime": "2020-01-16 17:22:49",
                            "issue_time": "2021-03-07 13:46:14",
                            "expire_time": null,
                            "is_ttl_refreshable": false,
                            "client_id": "b662b483feff40c1832e42cc1fcbc500",
                            "client_name": "sandbox-api",
                            "client_icon": "",
                            "client_homepage": "https:\\/\\/sandbox.yandex-team.ru\\/",
                            "client_ctime": "2015-04-07 11:48:23",
                            "client_is_yandex": false,
                            "xtoken_id": "",
                            "meta": ""
                          },
                          "status": {
                            "value": "VALID",
                            "id": 0
                          },
                          "error": "OK",
                          "uid": {
                            "value": "1120000000208259",
                            "lite": false,
                            "hosted": false
                          },
                          "login": "robot-ci-testing",
                          "have_password": true,
                          "have_hint": false,
                          "karma": {
                            "value": 0
                          },
                          "karma_status": {
                            "value": 0
                          },
                          "connection_id": "t:1040137"
                        }
                        """));
        assertEquals(
                new OAuthResponse("robot-ci-testing", new OAuthInfo(Set.of(
                        "oauth:grant_xtoken",
                        "sandbox:api",
                        "staff:read",
                        "startrek:write",
                        "arcanum:api_read",
                        "vault:use"
                ))),
                blackboxClient.getOAuth("127.0.0.1", "token")
        );
    }

}
