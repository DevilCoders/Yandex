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

import ru.yandex.ci.tools.potato.repo.arcadia.ArcadiaSearchResults;

public class ArcadiaFetcher implements Fetcher {
    private static final Logger log = LoggerFactory.getLogger(ArcadiaFetcher.class);

    private final HttpClient client = HttpClient.newBuilder()
            .followRedirects(HttpClient.Redirect.NORMAL)
            .build();

    // https://a.yandex-team.ru/oauth/token
    private static final String ARCANUM_TOKEN = System.getenv("ARCANUM_TOKEN");

    @Override
    public List<ConfigRef> findAll() {
        List<ConfigRef> configs = new ArrayList<>();
        configs.addAll(findFiles(ConfigRef.Type.YAML));
        configs.addAll(findFiles(ConfigRef.Type.JSON));
        return configs;
    }

    private List<ConfigRef> findFiles(ConfigRef.Type type) {
        log.info("Find {} in arcadia", type.getFilename());

        try {
            URI uri = new URIBuilder("https://cs.yandex-team.ru/search.py")
                    .addParameter("fileRegexp", type.getFilename().replace(".", "\\."))
                    .addParameter("max", "5000")
                    .addParameter("branch", "arcadia")
                    .addParameter("json", "on")
                    .addParameter("nocontrib", "on")
                    .build();

            HttpRequest request = HttpRequest.newBuilder()
                    .GET()
                    .uri(uri)
                    .build();


            HttpResponse<ArcadiaSearchResults> response = client.send(request,
                    JsonBodyHandler.to(ArcadiaSearchResults.class));
            ArcadiaSearchResults result = response.body();

            if (result.getStats().isOverflow()) {
                throw new RuntimeException("too many results");
            }

            log.info("Found {} {} files in arcadia", result.getResults().size(), type.getFilename());
            return result.getResults().stream()
                    .map(i -> fetchArcanumFile(i.getPath(), type))
                    .collect(Collectors.toList());

        } catch (URISyntaxException | IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public String getContent(String filepath) {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                    .GET()
                    .uri(new URIBuilder("https://a.yandex-team.ru/api/v2/repos/arc/downloads")
                            .addParameter("at", "trunk")
                            .addParameter("path", filepath)
                            .build()
                    )
                    .header("Authorization", "OAuth " + ARCANUM_TOKEN)
                    .build();

            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
            return response.body();
        } catch (URISyntaxException | IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    private ConfigRef fetchArcanumFile(String filepath, ConfigRef.Type type) {
        String content = getContent(filepath);
        return new ConfigRef(Sources.ARCADIA, type, filepath, content);
    }
}
