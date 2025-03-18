package ru.yandex.ci.core.spring.clients;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.tvm.TvmClientWrapper;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class TvmClientConfig {

    @Bean(destroyMethod = "close")
    public TvmClient tvmClient(
            @Value("${ci.tvmClient.selfTvmClientId}") int selfTvmClientId,
            @Value("${ci.tvmClient.selfTvmSecret}") String selfTvmSecret,
            @Value("${ci.tvmClient.blackboxEnv}") String blackboxEnv,
            @Autowired List<TvmTargetClientId> targetClientIds
    ) {
        int[] clientIds = targetClientIds.stream()
                .mapToInt(TvmTargetClientId::getId)
                .toArray();

        return TvmClientWrapper.getTvmClient(selfTvmClientId, selfTvmSecret, clientIds, blackboxEnv);
    }

}

