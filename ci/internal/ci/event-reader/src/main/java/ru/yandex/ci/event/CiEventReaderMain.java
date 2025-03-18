package ru.yandex.ci.event;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
public class CiEventReaderMain {

    public static void main(String[] args) {
        CiApplication.run(args, CiEventReaderMain.class, "ci", "ci-event-reader");
    }
}
