package ru.yandex.ci.storage.exporter;

import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration;

import ru.yandex.ci.common.application.CiApplication;

@SpringBootApplication(exclude = DataSourceAutoConfiguration.class)
@SuppressWarnings("HideUtilityClassConstructor")
public class CiStorageExporterMain {
    public static void main(String[] args) {
        CiApplication.run(args, CiStorageExporterMain.class, "ci-storage", "ci-storage-exporter");
    }
}
