package ru.yandex.ci.tools.potato;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PotatoFetcher {
    private static final Logger log = LoggerFactory.getLogger(PotatoFetcher.class);

    private static final Pattern KEY_PATTERN = Pattern.compile(
            "rootLoader\\.parsing\\.(yaml|json)\\.(?<service>[^/]+)/(?<project>[^/]+)/(?<repo>.+)"
    );
    private final Fetcher github;
    private final Fetcher arcadia;
    private final Fetcher bitbucket;

    private final HttpClient client = HttpClient.newBuilder()
            .build();

    public PotatoFetcher(Fetcher github, Fetcher arcadia, Fetcher bitbucket) {
        this.github = github;
        this.arcadia = arcadia;
        this.bitbucket = bitbucket;
    }

    public List<ConfigRef> findAll() {
        List<ConfigRef> configs = new ArrayList<>();
        configs.addAll(findAll(ConfigRef.Type.YAML));
        configs.addAll(findAll(ConfigRef.Type.JSON));
        return configs;
    }

    private List<ConfigRef> findAll(ConfigRef.Type type) {
        try {
            String suffix = switch (type) {
                case JSON -> "json";
                case YAML -> "yaml";
            };

            String uri = "https://potato.yandex-team.ru/telemetry/healthcheck?namespace=rootLoader.parsing.%s*"
                    .formatted(suffix);

            HttpRequest request = HttpRequest.newBuilder()
                    .GET()
                    .uri(URI.create(uri))
                    .build();

            HttpResponse<Map<String, Status>> response = client.send(request,
                    JsonBodyHandler.to(JsonMap.ofStringTo(Status.class)));
            Map<String, Status> content = response.body();

            log.info("Found {} {} configs in potato health", content.size(), type.getFilename());
            return content.keySet().stream()
                    .map(key -> getContent(key, type))
                    .collect(Collectors.toList());

        } catch (InterruptedException | IOException e) {
            throw new RuntimeException(e);
        }
    }

    private ConfigRef getContent(String key, ConfigRef.Type type) {
        Matcher matcher = KEY_PATTERN.matcher(key);
        if (!matcher.matches()) {
            throw new RuntimeException("key not matches: " + key);
        }
        String service = matcher.group("service");
        String project = matcher.group("project");
        String repo = matcher.group("repo");


        return switch (service) {
            case "bitbucket-enterprise" -> {
                var path = "https://bb.yandex-team.ru/projects/%s/repos/%s/raw/%s"
                        .formatted(project.toLowerCase(), repo.toLowerCase(), type.getFilename());
                var content = bitbucket.getContent(path);
                yield new ConfigRef(Sources.BB, type, path, content);
            }
            case "github-enterprise" -> {
                String defaultBranch = github.getDefaultBranch(project, repo);
                var path = "https://github.yandex-team.ru/%s/%s/blob/%s/%s"
                        .formatted(project.toLowerCase(), repo.toLowerCase(), defaultBranch, type.getFilename());
                var content = github.getContent(path);
                yield new ConfigRef(Sources.GITHUB, type, path, content);
            }
            case "bitbucket.browser-enterprise" -> {
                var path = "https://bitbucket.browser.yandex-team.ru/projects/%s/repos/%s/raw/%s"
                        .formatted(project.toLowerCase(), repo.toLowerCase(), type.getFilename());
                var content = bitbucket.getContent(path);
                yield new ConfigRef(Sources.BB_BROWSER, type, path, content);
            }
            case "arcanum" -> {
                var path = "%s/%s".formatted(repo, type.getFilename());
                var content = arcadia.getContent(path);
                yield new ConfigRef(Sources.ARCADIA, type, path, content);
            }
            default -> throw new RuntimeException("unexpected service: " + service + " key: " + key);
        };
    }

    @Value
    private static class Status {
        boolean healthy;
    }
}
