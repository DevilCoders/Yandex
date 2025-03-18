package ru.yandex.ci.common.ydb.spring;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Optional;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.google.common.base.Strings;
import com.google.common.net.HostAndPort;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;

import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.auth.KikimrAuthConfig;

/**
 * Uses YDB from
 * 1. ${ydb.kikimrConfig.ydbEndpoint}
 * 2. Receipt (when run from ya)
 * 3. Docker container (will be started automatically)
 */

/*
command for manual docker:
docker run -dp 2135:2135 -dp 2136:2136 -dp 8765:8765 --name ydb_junit registry.yandex.net/yandex-docker-local-ydb:latest
 */
@Slf4j
@Configuration
@Import(YdbJdbcConfig.class)
@PropertySource(value = "file:${user.home}/.ci/ci-test.properties", ignoreResourceNotFound = true)
public class YdbTestConfig {
    private static final Path RECIPE_DATABASE_FILE = Path.of("ydb_database.txt");
    private static final Path RECIPE_ENDPOINT_FILE = Path.of("ydb_endpoint.txt");

    private static final String LOCAL_PATH = "/local/";

    public String recipeEndpoint() {
        try {
            return Files.readString(RECIPE_ENDPOINT_FILE);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    public String recipeDatabase() {
        try {
            return Files.readString(RECIPE_DATABASE_FILE);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    private boolean isRecipe() {
        return Files.exists(RECIPE_ENDPOINT_FILE);
    }

    @Bean(destroyMethod = "shutdown")
    public ExecutorService testExecutor() {
        return Executors.newFixedThreadPool(16);
    }

    @Bean
    public KikimrConfig kikimrConfig(
            @Value("${ydb.kikimrConfig.ydbEndpoint:}") String ydbEndpoint,
            @Value("${ydb.kikimrConfig.ydbDatabase:}") String ydbDatabase,
            @Value("${ydb.kikimrConfig.ydbToken:}") String ydbToken,
            @Value("${ydb.kikimrConfig.tablespaceSuffix:}") String tablespaceSuffix
    ) {
        var maybeEndpoint = Optional.<HostAndPort>empty();

        if (System.getProperties().containsKey("ci.ydb.endpoint")) {
            maybeEndpoint = Optional.of(HostAndPort.fromString(System.getProperty("ci.ydb.endpoint")));
        } else if (!Strings.isNullOrEmpty(ydbEndpoint)) {
            maybeEndpoint = Optional.of(HostAndPort.fromString(ydbEndpoint));
        } else if (isRecipe()) {
            maybeEndpoint = Optional.of(HostAndPort.fromString(recipeEndpoint()));
        }

        if (maybeEndpoint.isPresent()) {
            var endpoint = maybeEndpoint.get();
            var tablespace = (ydbDatabase.isEmpty() ? LOCAL_PATH : ydbDatabase + "/local/") + tablespaceSuffix;
            var database = ydbDatabase.isEmpty() ? database() : ydbDatabase;

            log.info("Connecting to {}, tablespace: {}, database: {}", endpoint, tablespace, database);
            if (
                    !(isRecipe() ||
                            ydbDatabase.startsWith("/ru/home") ||
                            ydbDatabase.startsWith("/ru-prestable/home") ||
                            ydbDatabase.startsWith("/ru-prestable/ydb_home") ||
                            ydbDatabase.isEmpty())
            ) {
                System.err.println("Danger! Trying to run tests over stable database." +
                        " Endpoint: " + endpoint + "; database: " + ydbDatabase + ". Terminating");
                System.exit(1);
            }

            var config = KikimrConfig.create(endpoint.getHost(), endpoint.getPort(), tablespace, database);
            if (!ydbDatabase.isEmpty()) {
                config = config.withDiscoveryEndpoint(endpoint.toString());
            }

            if (!ydbToken.isEmpty()) {
                config = config.withAuth(KikimrAuthConfig.token(ydbToken));
            }

            return config;
        }

        throw new RuntimeException("No recipe or YDB configuration found. " +
                "See: https://a.yandex-team.ru/arcadia/ci/README.md#tests");
    }

    public String database() {
        if (isRecipe()) {
            return "/" + StringUtils.strip(recipeDatabase(), "/");
        } else {
            return "/local";
        }
    }
}
