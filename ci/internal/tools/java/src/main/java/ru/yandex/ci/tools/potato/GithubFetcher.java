package ru.yandex.ci.tools.potato;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import org.apache.http.client.utils.URIBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.tools.potato.repo.github.GithubSearchResults;
import ru.yandex.ci.tools.potato.repo.github.Item;
import ru.yandex.ci.tools.potato.repo.github.Repo;

public class GithubFetcher implements Fetcher {
    private static final Logger log = LoggerFactory.getLogger(GithubFetcher.class);

    private final HttpClient client = HttpClient.newBuilder()
            .followRedirects(HttpClient.Redirect.NORMAL)
            .build();

    @Override
    public List<ConfigRef> findAll() {
        List<ConfigRef> configs = new ArrayList<>();
        configs.addAll(findFiles(ConfigRef.Type.YAML));
        configs.addAll(findFiles(ConfigRef.Type.JSON));
        return configs;
    }

    @Override
    public String getContent(String path) {
        try {
            String url = path.replace("/blob/", "/raw/");
            HttpRequest request = HttpRequest.newBuilder().GET().uri(URI.create(url)).build();

            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() != 200) {
                throw new RuntimeException("error fetching file " + path + ": " + response);
            }
            return response.body();

        } catch (IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    private List<ConfigRef> findFiles(ConfigRef.Type type) {
        String filename = type.getFilename();
        log.info("Find {} in github", filename);
        try {
            int pageSize = 100;
            HttpRequest request = HttpRequest.newBuilder()
                    .GET()
                    .uri(new URIBuilder("https://api.github.yandex-team.ru/search/code")
                            .addParameter("q", "filename:" + filename)
                            .addParameter("per_page", String.valueOf(pageSize))
                            .build()
                    )
                    .build();

            HttpResponse<GithubSearchResults> response = client.send(request,
                    JsonBodyHandler.to(GithubSearchResults.class));
            GithubSearchResults results = response.body();

            if (results.isIncompleteResults() || results.getItems().size() == pageSize) {
                throw new RuntimeException("too many results: " + results);
            }

            log.info("Found {} {} files in github", results.getItems().size(), filename);
            return results.getItems().stream()
                    .filter(this::isRepoActive)
                    .map(item -> this.fetchGithubFile(item, type))
                    .collect(Collectors.toList());

        } catch (InterruptedException | IOException | URISyntaxException e) {
            throw new RuntimeException(e);
        }
    }

    private ConfigRef fetchGithubFile(Item item, ConfigRef.Type type) {
        String url = normalizeUrl(item);
        String content = getContent(url);
        return new ConfigRef("github", type, url, content);
    }

    private boolean isRepoActive(Item item) {
        try {
            String project = item.getRepository().getOwner().getLogin();
            String repo = item.getRepository().getName();
            HttpRequest request = HttpRequest.newBuilder()
                    .GET()
                    .uri(new URIBuilder("https://api.github.yandex-team.ru")
                            .setPathSegments("repos", project, repo)
                            .build()
                    )
                    .build();

            HttpResponse<Repo> response = client.send(request, JsonBodyHandler.to(Repo.class));
            return !response.body().isArchived();
        } catch (URISyntaxException | IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public String getDefaultBranch(String project, String repo) {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                    .GET()
                    .uri(new URIBuilder("https://api.github.yandex-team.ru")
                            .setPathSegments("repos", project, repo)
                            .build()
                    )
                    .build();

            HttpResponse<Repo> response = client.send(request, JsonBodyHandler.to(Repo.class));
            return response.body().getDefaultBranch();
        } catch (URISyntaxException | IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    private String normalizeUrl(Item item) {
        String defaultBranch = getDefaultBranch(item.getRepository().getOwner().getLogin(),
                item.getRepository().getName());

        return "https://github.yandex-team" +
                ".ru/%s/%s/blob/%s/%s".formatted(
                        item.getRepository().getOwner().getLogin().toLowerCase(),
                        item.getRepository().getName().toLowerCase(),
                        defaultBranch,
                        item.getPath()
                );
    }
}
