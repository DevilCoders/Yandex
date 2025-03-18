package ru.yandex.ci.observer.reader;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
public class CiObserverReaderMain {
    public static void main(String[] args) {
        CiApplication.run(args, CiObserverReaderMain.class, "ci-observer", "ci-observer-reader");
    }
}
