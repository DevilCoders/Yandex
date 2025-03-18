package ru.yandex.ci.flow.spring;

import com.google.common.base.Splitter;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.commune.zk2.ZkConfiguration;
import ru.yandex.commune.zk2.ZkPath;
import ru.yandex.commune.zk2.client.ZkManager;
import ru.yandex.commune.zk2.primitives.observer.ZkPathObserver;

public class ZkConfig {

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ZkConfiguration zkConfiguration(@Value("${ci.zkConfiguration.zkHosts}") String zkHosts) {
        return new ZkConfiguration(Splitter.on(',').trimResults().omitEmptyStrings().splitToList(zkHosts));
    }

    @Bean
    public ZkPath zkPath(@Value("${ci.zkPath.path}") String path) {
        return new ZkPath(path);
    }

    @Bean
    public ZkPathObserver zkPathObserver(ZkPath zkPath) {
        return new ZkPathObserver(zkPath);
    }

    @Bean
    public ZkManager zkManager(ZkConfiguration zkConfiguration,
                               ZkPathObserver zkPathObserver) {
        ZkManager zkManager = ZkManager.consAndStart(zkConfiguration);
        zkManager.addClient(zkPathObserver);
        zkManager.waitUntilInitializedForTesting(); //Иначе возникает Race Condition при создании ноды в ЗК
        return zkManager;
    }
}
