package ru.yandex.ci.client.blackbox;

public interface BlackboxClient {

    UserInfoResponse getUserInfo(String userIp, long uid);

    OAuthResponse getOAuth(String userIp, String token);
}
