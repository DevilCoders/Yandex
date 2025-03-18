package ru.yandex.amazonaws.auth;

import java.util.Objects;
import java.util.concurrent.atomic.AtomicReference;

import com.amazonaws.auth.AWSRefreshableSessionCredentials;
import com.amazonaws.auth.AWSSessionCredentials;
import com.amazonaws.auth.AWSSessionCredentialsProvider;

import ru.yandex.passport.tvmauth.TvmClient;

/**
 * <p>
 * Стандартная имплементация AWS Credentials Provider для Яндекс-сервисов, совместимых с Amazon AWS (SQS, S3, etc)
 * </p>
 *
 * <p>
 * Пример конфигурации для Amazon SQS:
 * <pre>
 * ...
 * import ru.yandex.passport.tvmauth.NativeTvmClient;
 * import ru.yandex.passport.tvmauth.TvmApiSettings;
 * import com.amazonaws.services.sqs.AmazonSQS;
 * import com.amazonaws.services.sqs.AmazonSQSClientBuilder;
 * ...
 *
 *         // Код сервиса SQS over KiKiMR, совместимый с Amazon SQS: https://wiki.yandex-team.ru/sqs/
 *         final int SQS_PRODUCTION_ID = 2002456;
 *
 *         final String endpoint = "http://sqs.yandex.net:8771";
 *         final String region = "eu-west-1";
 *
 *         final String accessKey = ...; // Ваш аккаунт в SQS over KiKiMR
 *         final String secretKey = "unused";
 *
 *         final int clientId = ...; // ID вашего сервиса в https://abc.yandex-team.ru/resources/
 *         final String secret = ...; // Секрет вашего сервиса в https://abc.yandex-team.ru/resources/
 *
 *         final NativeTvmClient client = new NativeTvmClient(new TvmApiSettings()
 *                 .setSelfTvmId(clientId)
 *                 .enableServiceTicketsFetchOptions(secret, new int[]{SQS_PRODUCTION_ID}));
 *
 *         final AmazonSQS sqs = AmazonSQSClientBuilder.standard()
 *                 .withEndpointConfiguration(new AwsClientBuilder.EndpointConfiguration(endpoint, region))
 *                 .withCredentials(new TVM2AWSCredentialsProvider(client, accessKey, secretKey, SQS_PRODUCTION_ID))
 *                 .build();
 * </pre>
 * </p>
 */
public class TVM2AWSCredentialsProvider implements AWSSessionCredentialsProvider {

    private final TvmClient tvmClient;
    private final String awsAccessKey;
    private final String awsSecretKey;
    private final int tvmDestinationClientId;

    /**
     * @param tvmClient    экземпляр класса NativeTvmClient, настроенный на работу со стандартным или локальным
     *                     прокси-сервисом. Клиент должен быть сконфигурирован на запрос токенов для того же сервиса,
     *                     что и параметр {@link #tvmDestinationClientId}
     * @param awsAccessKey ключ доступа AWS
     * @param awsSecretKey секретный ключ AWS
     * @param tvmDestinationClientId    уникальный идентификатор сервиса, к которому мы хотим подключиться. Один из
     *                     https://abc.yandex-team.ru/resources/
     * @link ru.yandex.passport.tvmauth.TvmApiSettings#enableServiceTicketsFetchOptions(String, int[])
     */
    public TVM2AWSCredentialsProvider(TvmClient tvmClient, String awsAccessKey, String awsSecretKey,
                                      int tvmDestinationClientId) {
        this.tvmClient = Objects.requireNonNull(tvmClient, "tvmClient cannot be null");
        this.awsAccessKey = Objects.requireNonNull(awsAccessKey, "awsAccessKey cannot be null");
        this.awsSecretKey = Objects.requireNonNull(awsSecretKey, "awsSecretKey cannot be null");
        this.tvmDestinationClientId = tvmDestinationClientId;
    }

    @Override
    public AWSSessionCredentials getCredentials() {
        return new TVM2RefreshableSessionCredentials();
    }

    @Override
    public void refresh() {
        // Ключ уже рефрешится в классе TVM2RefreshableSessionCredentials
    }

    private class TVM2RefreshableSessionCredentials implements AWSRefreshableSessionCredentials {

        private final AtomicReference<String> token = new AtomicReference<>();

        private TVM2RefreshableSessionCredentials() {
            this.refreshCredentials();
        }

        @Override
        public void refreshCredentials() {
            this.token.set(tvmClient.getServiceTicketFor(tvmDestinationClientId));
        }

        @Override
        public String getAWSAccessKeyId() {
            return awsAccessKey;
        }

        @Override
        public String getAWSSecretKey() {
            return awsSecretKey;
        }

        @Override
        public String getSessionToken() {
            return token.get();
        }
    }
}
