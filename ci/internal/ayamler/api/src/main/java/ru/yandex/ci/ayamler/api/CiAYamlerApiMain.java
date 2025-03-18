package ru.yandex.ci.ayamler.api;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
public class CiAYamlerApiMain {

    public static void main(String[] args) {
        CiApplication.run(args, CiAYamlerApiMain.class, "ci-ayamler", "ci-ayamler-api");
    }
}
