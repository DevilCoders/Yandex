package ru.yandex.ci.storage.shard;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.data.jdbc.JdbcRepositoriesAutoConfiguration;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = {
        DataSourceAutoConfiguration.class,
        JdbcRepositoriesAutoConfiguration.class
})
@SuppressWarnings("HideUtilityClassConstructor")
public class CiStorageShardMain {

    public static void main(String[] args) {
        CiApplication.run(args, CiStorageShardMain.class, "ci-storage", "ci-storage-shard");
    }
}
