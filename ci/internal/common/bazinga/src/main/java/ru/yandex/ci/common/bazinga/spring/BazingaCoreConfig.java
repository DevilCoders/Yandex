package ru.yandex.ci.common.bazinga.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.BazingaTaskManagerImpl;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDb;
import ru.yandex.commune.bazinga.ydb.storage.YdbBazingaStorage;


// Required database configuration in same context
@Configuration
public class BazingaCoreConfig {

    @Bean
    public BazingaStorage bazingaStorage(BazingaStorageDb db) {
        return new YdbBazingaStorage(db);
    }

    @Bean
    public BazingaTaskManager bazingaTaskManager(BazingaStorage bazingaStorage) {
        return new BazingaTaskManagerImpl(bazingaStorage);
    }
}
