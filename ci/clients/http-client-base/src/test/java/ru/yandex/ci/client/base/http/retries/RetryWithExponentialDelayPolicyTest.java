package ru.yandex.ci.client.base.http.retries;

import java.time.Duration;

import okhttp3.Request;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.junit.jupiter.MockitoExtension;
import retrofit2.Response;

import static org.assertj.core.api.Assertions.assertThat;

@ExtendWith(MockitoExtension.class)
public class RetryWithExponentialDelayPolicyTest {

    @Test
    public void retryDelays() {
        var request = new Request.Builder()
                .url("http://localhost")
                .build();

        var response = Response.success("OK");

        var policy = new RetryWithExponentialDelayPolicy(6, Duration.ofMillis(1), Duration.ofMillis(9), 2);
        assertThat(policy.canRetry(request, response, 2)).isEqualTo(1);
        assertThat(policy.canRetry(request, response, 3)).isEqualTo(2);
        assertThat(policy.canRetry(request, response, 4)).isEqualTo(4);
        assertThat(policy.canRetry(request, response, 5)).isEqualTo(8);
        assertThat(policy.canRetry(request, response, 6)).isEqualTo(9);
        assertThat(policy.canRetry(request, response, 7)).isEqualTo(-1);
    }
}
