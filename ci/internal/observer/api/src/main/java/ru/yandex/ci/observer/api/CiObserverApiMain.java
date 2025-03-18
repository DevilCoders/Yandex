package ru.yandex.ci.observer.api;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;
import org.springframework.scheduling.annotation.EnableScheduling;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
@EnableScheduling
public class CiObserverApiMain {
    public static void main(String[] args) {
        CiApplication.run(args, CiObserverApiMain.class, "ci-observer", "ci-observer-api");
    }
}
