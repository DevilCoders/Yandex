package ru.yandex.ci.storage.tms;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
public class CiStorageTmsMain {
    public static void main(String[] args) {
        CiApplication.run(args, CiStorageTmsMain.class, "ci-storage", "ci-storage-tms");
    }
}
