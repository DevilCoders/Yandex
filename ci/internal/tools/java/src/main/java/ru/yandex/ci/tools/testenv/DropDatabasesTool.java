package ru.yandex.ci.tools.testenv;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import com.beust.jcommander.Parameter;
import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.http.HttpStatus;

import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.testenv.TestenvClient.Confirmation;
import ru.yandex.ci.client.testenv.model.TestenvProject;
import ru.yandex.ci.engine.spring.clients.TestenvClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import({
        TestenvClientConfig.class
})
public class DropDatabasesTool extends AbstractSpringBasedApp {

    @Autowired
    private TestenvClient testenvClient;

    @Parameter(names = "--db-names-filepath", required = true)
    private Path dbNamesFilePath;

    @Parameter(names = "--do")
    private boolean doAction;

    @Override
    protected void run() throws Exception {
        List<String> dbNames = readDatabaseNamesFromFile();
        log.info("Load {} database names", dbNames.size());

        for (String dbName : dbNames) {
            dropDatabase(dbName);
        }
    }

    private List<String> readDatabaseNamesFromFile() throws IOException {
        try (var stream = Files.lines(dbNamesFilePath)) {
            return stream
                    .map(String::strip)
                    .toList();
        }
    }

    private void dropDatabase(String dbName) {
        log.info("Processing {}", dbName);
        try {
            var project = testenvClient.getProject(dbName);
            log.info("Database {} status {}", dbName, project.getStatus());

            Preconditions.checkState(project.getStatus() == TestenvProject.Status.STOPPED,
                    "project %s status %s", dbName, project.getStatus());

            if (doAction) {
                testenvClient.dropProject(dbName, Confirmation.I_UNDERSTAND_THAT_THIS_METHOD_IS_DANGEROUS);
                log.info("Database {} dropped", dbName);
            } else {
                log.info("Action not performed, dry-run");
            }

        } catch (HttpException e) {
            if (e.getHttpCode() == HttpStatus.NOT_FOUND.value()) {
                log.info("Database {} not found", dbName);
            } else {
                throw e;
            }
        }
        log.info("Processed {}", dbName);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
