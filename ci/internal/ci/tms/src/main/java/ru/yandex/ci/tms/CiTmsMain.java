package ru.yandex.ci.tms;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
public class CiTmsMain {

    public static void main(String[] args) {
        CiApplication.run(args, CiTmsMain.class, "ci", "ci-tms");
    }
}
