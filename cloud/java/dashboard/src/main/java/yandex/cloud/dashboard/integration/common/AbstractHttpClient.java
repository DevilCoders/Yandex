package yandex.cloud.dashboard.integration.common;

import lombok.SneakyThrows;
import org.apache.http.client.HttpClient;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.conn.ssl.NoopHostnameVerifier;
import org.apache.http.conn.ssl.TrustAllStrategy;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.ssl.SSLContextBuilder;

import java.time.Duration;

/**
 * @author ssytnik
 */
public class AbstractHttpClient {
    private static final int HTTP_CLIENT_MAX_CONNECTION_COUNT = 5;
    private static final Duration HTTP_CLIENT_TIMEOUT = Duration.ofSeconds(100);

    protected HttpClient httpClient = createHttpClient();
    protected String host;

    public AbstractHttpClient(String host) {
        this.host = host;
    }

    @SneakyThrows
    protected static HttpClient createHttpClient() {
        return HttpClientBuilder.create()
                .setSSLContext(new SSLContextBuilder().loadTrustMaterial(new TrustAllStrategy()).build())
                .setSSLHostnameVerifier(NoopHostnameVerifier.INSTANCE)
                .setMaxConnTotal(HTTP_CLIENT_MAX_CONNECTION_COUNT)
                .setDefaultRequestConfig(
                        RequestConfig.copy(RequestConfig.DEFAULT)
                                .setConnectTimeout((int) HTTP_CLIENT_TIMEOUT.toMillis())
                                .setSocketTimeout((int) HTTP_CLIENT_TIMEOUT.toMillis())
                                .build()
                )
                .build();
    }
}
