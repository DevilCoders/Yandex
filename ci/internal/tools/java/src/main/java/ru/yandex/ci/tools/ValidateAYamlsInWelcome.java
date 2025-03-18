package ru.yandex.ci.tools;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import lombok.Value;
import org.apache.commons.vfs2.VFS;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.validation.ValidationReport;

public class ValidateAYamlsInWelcome {
    private static final Logger log = LoggerFactory.getLogger(ValidateAYamlsInWelcome.class);

    private static final Path ATTACHMENTS_PATH = Path.of(
            System.getenv("ATTACHMENTS_PATH") // $ARCADIA_ROOT/ci/tools/py/download_configs/attachments
    );

    public static void main(String[] args) throws Exception {
        ValidateAYamlsInWelcome validate = new ValidateAYamlsInWelcome();
        validate.run();
    }

    private void run() throws Exception {
        List<YamlFile> yamls = scanAYamls();

        int invalid = 0;
        for (YamlFile yamlFile : yamls) {
            log.info("Checking {}...", yamlFile.getPath());
            if (!isValid(yamlFile)) {
                log.info("Invalid: {}", yamlFile.getPath());
                invalid++;
            }
        }

        if (invalid > 0) {
            log.error("{} of {} files invalid", invalid, yamls.size());
        } else {
            log.info("All {} files are valid", yamls.size());
        }
    }

    private boolean isValid(YamlFile yamlFile) {
        ValidationReport report;
        try {
            var content = yamlFile.getContent();
            content = content.replace("#your ci secret#", "sec-12")
                    .replace("#your service name#", "ci")
                    .replace("#your sandbox group#", "robot-test-ci");
            report = AYamlParser.parseAndValidate(content);
        } catch (Exception e) {
            log.error("{} unable to parse: {}", yamlFile.getPath(), e);
            return false;
        }
        if (!report.isSuccess()) {
            log.error("{} validation error: {}", yamlFile.getPath(), report);
            return false;
        }

        return true;
    }

    @Value
    private static class YamlFile {
        Path path;
        String content;
    }

    private List<YamlFile> scanAYamls() throws IOException {
        var archives = scanTarGz();
        var manager = VFS.getManager();
        List<YamlFile> result = new ArrayList<>();
        for (Path archive : archives) {
            var pathInArch = "tar:file://" + ATTACHMENTS_PATH.resolve(archive.toString()) + "!/a.yaml";
            var fileObject = manager.resolveFile(pathInArch);
            var yamlContent = fileObject.getContent().getString(StandardCharsets.UTF_8);
            result.add(new YamlFile(archive, yamlContent));
        }
        return result;
    }

    @SuppressWarnings("StreamResourceLeak")
    private List<Path> scanTarGz() throws IOException {
        log.info("Scanning files locally");
        return Files.walk(ATTACHMENTS_PATH)
                .filter(Files::isRegularFile)
                .filter(path -> path.toString().endsWith(".tar.gz")) // на самом деле это нифига не gzip
                .peek(path -> log.info("Found {}", path))
                .collect(Collectors.toList());
    }

}
