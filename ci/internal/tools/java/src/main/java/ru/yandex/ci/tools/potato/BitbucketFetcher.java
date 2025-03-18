package ru.yandex.ci.tools.potato;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class BitbucketFetcher implements Fetcher {
    private static final Logger log = LoggerFactory.getLogger(BitbucketFetcher.class);

    // https://bb.yandex-team.ru/plugins/servlet/access-tokens/manage
    // https://bitbucket.browser.yandex-team.ru/plugins/servlet/access-tokens/manage
    private static final String TOKEN = System.getenv("BB_TOKEN");
    private static final String BROWSER_TOKEN = System.getenv("BB_BROWSER_TOKEN");

    private final HttpClient client = HttpClient.newBuilder()
            .followRedirects(HttpClient.Redirect.NORMAL)
            .build();


    @Override
    public List<ConfigRef> findAll() {
        List<ConfigRef> configs = Stream.of(
                "https://bb.yandex-team.ru/projects/afisha/repos/ats/raw/.potato.json",
                "https://bb.yandex-team.ru/projects/afisha/repos/afisha-frontend/raw/.potato.json",
                "https://bb.yandex-team.ru/projects/eddl-mobdev/repos/client-ios/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/afisha/repos/domiki/raw/.potato.json",
                "https://bb.yandex-team.ru/projects/inforsce/repos/infor-sce/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/eddl-mobdev/repos/test_repo/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/ms/repos/kinopoisk-ios/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/kinopoisk/repos/kinopoisk-android/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/ms/repos/kinopoisk-player-ios/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/ms/repos/kinopoisk-mobile/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/music-mobile/repos/mobile-music-uwp/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/music-mobile/repos/mobile-music-ios/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/music-mobile/repos/mobile-stories-sdk-ios/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/music-mobile/repos/mobile-stories-sdk-android/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/music/repos/frontend-mono/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/music-mobile/repos/music-mobile-android-music/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/plus/repos/plus-front/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/ott/repos/frontend/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/sloy/repos/android-app/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/sdc/repos/www/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/taxi/repos/mobile-taxi-client-android/raw/.potato.yml",
                "https://bb.yandex-team.ru/projects/taxi/repos/design-components-android/raw/.potato.yml"
        )
                .map(this::fetch)
                .collect(Collectors.toList());

        log.info("Found {} configs in bb, hardcoded", configs.size());
        return configs;
    }

    @Override
    public String getContent(String path) {
        try {
            String tokenToUse = isBrowserBB(path) ? BROWSER_TOKEN : TOKEN;
            HttpRequest request = HttpRequest.newBuilder()
                    .GET()
                    .uri(URI.create(path))
                    .header("Authorization", "Bearer " + tokenToUse)
                    .build();

            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());
            return response.body();
        } catch (IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    private boolean isBrowserBB(String path) {
        return path.contains("bitbucket.browser.yandex-team.ru");
    }

    private ConfigRef fetch(String path) {
        String content = getContent(path);
        String type = isBrowserBB(path) ? Sources.BB_BROWSER : Sources.BB;
        return new ConfigRef(type, ConfigRef.Type.YAML, path, content);
    }
}
