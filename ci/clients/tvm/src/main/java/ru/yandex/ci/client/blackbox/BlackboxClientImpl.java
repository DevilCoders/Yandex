package ru.yandex.ci.client.blackbox;

import java.util.List;

import lombok.Value;
import retrofit2.http.GET;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class BlackboxClientImpl implements BlackboxClient {

    private final BlackboxApi api;

    private BlackboxClientImpl(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(BlackboxApi.class);
    }

    public static BlackboxClientImpl create(HttpClientProperties httpClientProperties) {
        return new BlackboxClientImpl(httpClientProperties);
    }

    @Override
    public UserInfoResponse getUserInfo(String userIp, long uid) {
        var response = api.userInfo(userIp, uid);
        return response.getUsers().isEmpty()
                ? new UserInfoResponse(null)
                : response.getUsers().get(0);
    }

    @Override
    public OAuthResponse getOAuth(String userIp, String token) {
        return api.oAuth(userIp, token);
    }


    @Value
    static class UsersResponse {
        List<UserInfoResponse> users;
    }

    interface BlackboxApi {
        @GET("/blackbox?method=userinfo&format=json")
        UsersResponse userInfo(
                @Query("userip") String userIp,
                @Query("uid") long uid
        );

        @GET("/blackbox?method=oauth&format=json")
        OAuthResponse oAuth(
                @Query("userip") String userIp,
                @Query("oauth_token") String token
        );
    }
}
