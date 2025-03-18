package ru.yandex.ci.api;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
public class CiApiMain {

    public static void main(String[] args) {
        CiApplication.run(args, CiApiMain.class, "ci", "ci-api");
    }
}
