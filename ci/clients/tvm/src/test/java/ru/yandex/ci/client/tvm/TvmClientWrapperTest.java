package ru.yandex.ci.client.tvm;

import org.junit.jupiter.api.Test;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.passport.tvmauth.exception.NonRetriableException;
import ru.yandex.passport.tvmauth.exception.RetriableException;

public class TvmClientWrapperTest {
    @Test
    public void testGetTvmClient() {
        TvmClient tvmClient;
        try {
            tvmClient = TvmClientWrapper.getTvmClient(123, "secret", new int[]{123}, null);
        } catch (Exception e) {
            if (e instanceof NonRetriableException
                    && e.getMessage().contains(
                    "Signature is bad: common reason is bad tvm_secret or tvm_id\\/tvm_secret mismatch."
            )) {
                return;
            } else if (e instanceof RetriableException && e.getMessage().contains("Failed to start TvmClient")) {
                return;
            } else {
                throw e;
            }
        }
        tvmClient.close();
    }
}
