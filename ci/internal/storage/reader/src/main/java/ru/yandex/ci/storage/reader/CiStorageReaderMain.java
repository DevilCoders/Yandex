package ru.yandex.ci.storage.reader;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.data.jdbc.JdbcRepositoriesAutoConfiguration;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = {
        DataSourceAutoConfiguration.class,
        JdbcRepositoriesAutoConfiguration.class
})
@SuppressWarnings("HideUtilityClassConstructor")
public class CiStorageReaderMain {
    public static void main(String[] args) {
        System.setProperty("spring.main.allow-circular-references", "true"); //CI-3210
        CiApplication.run(args, CiStorageReaderMain.class, "ci-storage", "ci-storage-reader");
    }
}
