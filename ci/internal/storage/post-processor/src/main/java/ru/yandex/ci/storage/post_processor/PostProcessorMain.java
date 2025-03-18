package ru.yandex.ci.storage.post_processor;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.data.jdbc.JdbcRepositoriesAutoConfiguration;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = {
        DataSourceAutoConfiguration.class,
        JdbcRepositoriesAutoConfiguration.class
})
@SuppressWarnings("HideUtilityClassConstructor")
public class PostProcessorMain {
    public static void main(String[] args) {
        CiApplication.run(args, PostProcessorMain.class, "ci-storage", "ci-storage-post-processor");
    }
}
