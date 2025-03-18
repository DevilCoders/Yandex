package ru.yandex.ci.client.yav;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import lombok.extern.slf4j.Slf4j;
import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.Field;
import retrofit2.http.FormUrlEncoded;
import retrofit2.http.GET;
import retrofit2.http.Header;
import retrofit2.http.POST;
import retrofit2.http.Path;
import retrofit2.http.Tag;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.Idempotent;
import ru.yandex.ci.client.base.http.LoggingConfig;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmHeaders;
import ru.yandex.ci.client.yav.model.DelegatingTokenResponse;
import ru.yandex.ci.client.yav.model.YavResponse;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.client.yav.model.YavSecretsResponse;
import ru.yandex.ci.client.yav.model.YavTokenizedRequest;

@Slf4j
public class YavClientImpl implements YavClient {

    private final YavApi api;
    private final TvmAuthProvider tvmAuthProvider;
    private final int selfTvmClientId;

    private YavClientImpl(HttpClientProperties httpClientProperties, int selfTvmClientId) {
        this.tvmAuthProvider = Objects.requireNonNull((TvmAuthProvider) httpClientProperties.getAuthProvider());

        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .snakeCaseNaming()
                .build(YavApi.class);

        this.selfTvmClientId = selfTvmClientId;
    }

    public static YavClientImpl create(HttpClientProperties httpClientProperties, int selfTvmClientId) {
        return new YavClientImpl(httpClientProperties, selfTvmClientId);
    }

    @Override
    public YavSecret getSecretByDelegatingToken(
            String token,
            String secretVersion,
            String signature
    ) {
        YavTokenizedRequest request = new YavTokenizedRequest(List.of(
                YavTokenizedRequest.TokenizedRequest.builder()
                        .token(token)
                        .secretVersion(secretVersion)
                        .signature(signature)
                        .serviceTicket(tvmAuthProvider.getServiceTicket())
                        .build()
        ));

        YavSecretsResponse secrets = api.getSecrets(request, LoggingConfig.none());

        Preconditions.checkState(!secrets.getSecrets().isEmpty(), "No available secrets found for delegating token");

        YavSecret secret = secrets.getSecrets().get(0);

        checkResponseStatus(secret, "No available secrets found for delegating token");

        return secret;
    }

    @Override
    public DelegatingTokenResponse createDelegatingToken(
            String tvmUserTicket,
            String secretUuid,
            String signature,
            String comment
    ) {
        DelegatingTokenResponse response = api.delegateTokens(
                secretUuid,
                selfTvmClientId,
                encodeValue(signature),
                encodeValue(comment),
                tvmUserTicket,
                LoggingConfig.none()
        );

        Preconditions.checkState(response != null, "Can not get delegating token, cause: no response");
        checkResponseStatus(response, "Can not get delegating token");

        return response;
    }

    @Override
    public String getReaders(String secretUuid) {
        var response = api.getReaders(secretUuid);
        return response.body().toString();
    }

    @VisibleForTesting
    static void checkResponseStatus(YavResponse response, String errorMessage) {
        Preconditions.checkState(
                response.getStatus() != YavResponse.Status.ERROR,
                "%s, response: %s",
                errorMessage, response
        );

        if (response.getStatus() == YavResponse.Status.WARNING) {
            log.warn("Warning in yav response: {}", response);
        }
    }

    @Nullable
    private static String encodeValue(String value) {
        try {
            if (Strings.isNullOrEmpty(value)) {
                return null;
            } else {
                return URLEncoder.encode(value, StandardCharsets.UTF_8.toString());
            }
        } catch (UnsupportedEncodingException ex) {
            throw new RuntimeException(ex.getCause());
        }
    }

    interface YavApi {

        @Idempotent
        @POST("/1/tokens/")
        YavSecretsResponse getSecrets(
                @Body YavTokenizedRequest request,
                @Tag LoggingConfig loggingConfig
        );

        @FormUrlEncoded
        @POST("/1/secrets/{secretUuid}/tokens/")
        DelegatingTokenResponse delegateTokens(
                @Path("secretUuid") String secretUuid,
                @Field("tvm_client_id") int tvmClientId,
                @Field(value = "signature", encoded = true) String signature,
                @Field(value = "comment", encoded = true) String comment,
                @Header(TvmHeaders.USER_TICKET) String userTicket,
                @Tag LoggingConfig loggingConfig
        );

        @GET("/1/secrets/{secretUuid}/readers/")
        Response<Object> getReaders(@Path("secretUuid") String secretUuid);

    }

}
