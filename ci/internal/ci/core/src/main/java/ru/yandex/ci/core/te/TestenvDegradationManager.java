package ru.yandex.ci.core.te;

import java.time.LocalDateTime;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import lombok.ToString;
import lombok.Value;
import retrofit2.Response;
import retrofit2.http.GET;
import retrofit2.http.Path;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class TestenvDegradationManager {
    private final TestEnvApi api;

    private TestenvDegradationManager(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(TestEnvApi.class);
    }

    public static TestenvDegradationManager create(HttpClientProperties httpClientProperties) {
        return new TestenvDegradationManager(httpClientProperties);
    }

    public PlatformsStatuses getPrecommitPlatformStatuses() {
        var response = api.getPrecommitServiceLevel();
        return resolvePlatformStatuses(response);
    }

    public void manageDegradation(FallbackMode fallbackMode) {
        api.handler(fallbackMode.getPathName());
    }

    private static PlatformsStatuses resolvePlatformStatuses(PlatformsStatusesResponse response) {
        Map<Platform, PlatformStatus> platforms = response.platforms.stream()
                .filter(s -> !s.getName().equalsIgnoreCase("ALL_PLATFORMS"))
                .collect(Collectors.toMap(
                        s -> Platform.valueOf(s.getName().toUpperCase()),
                        Function.identity()
                ));

        Optional<PlatformStatus> allPlatformsStatus = response.getPlatforms().stream()
                .filter(s -> s.getName().equalsIgnoreCase("ALL_PLATFORMS"))
                .findFirst();

        if (allPlatformsStatus.isEmpty() || allPlatformsStatus.get().getStatus() == Status.enabled) {
            return new PlatformsStatuses(platforms);
        }

        platforms.values().forEach(s -> {
            s.setStatus(Status.disabled);
            if (s.getStatusChangeDateTime().isBefore(allPlatformsStatus.get().getStatusChangeDateTime())) {
                s.setStatusChangeDateTime(allPlatformsStatus.get().getStatusChangeDateTime());
                s.setStatusChangeLogin(allPlatformsStatus.get().getStatusChangeLogin());
            }
        });

        return new PlatformsStatuses(platforms);
    }

    public enum Status {
        enabled,
        disabled
    }

    public enum FallbackMode {
        ENABLE_FALLBACK_SANITIZERS("enableFallbackModeSanitizers"),
        ENABLE_FALLBACK_IOS_ANDROID_CYGWIN("enableFallbackModeIosAndroidCygwin"),
        ENABLE_FALLBACK_GCC_MVC_MUSL("enableFallbackModeGccMsvcMusl"),
        DISABLE_FALLBACK_SANITIZERS("disableFallbackModeSanitizers"),
        DISABLE_FALLBACK_IOS_ANDROID_CYGWIN("disableFallbackModeIosAndroidCygwin"),
        DISABLE_FALLBACK_GCC_MSVC_MUSL("disableFallbackModeGccMsvcMusl");

        private final String pathName;

        FallbackMode(String pathName) {
            this.pathName = pathName;
        }

        public String getPathName() {
            return pathName;
        }
    }

    public enum Platform {
        GCC_MSVC_MUSL(
                FallbackMode.ENABLE_FALLBACK_GCC_MVC_MUSL,
                FallbackMode.DISABLE_FALLBACK_GCC_MSVC_MUSL
        ),
        IOS_ANDROID_CYGWIN(
                FallbackMode.ENABLE_FALLBACK_IOS_ANDROID_CYGWIN,
                FallbackMode.DISABLE_FALLBACK_IOS_ANDROID_CYGWIN
        ),
        SANITIZERS(
                FallbackMode.ENABLE_FALLBACK_SANITIZERS,
                FallbackMode.DISABLE_FALLBACK_SANITIZERS
        );

        private final FallbackMode enableFallback;
        private final FallbackMode disableFallback;

        Platform(FallbackMode enableFallback, FallbackMode disableFallback) {
            this.enableFallback = enableFallback;
            this.disableFallback = disableFallback;
        }

        public FallbackMode enableFallbackAction() {
            return enableFallback;
        }

        public FallbackMode disableFallbackAction() {
            return disableFallback;
        }
    }

    @ToString
    public static class PlatformsStatuses {
        private final Map<Platform, PlatformStatus> platforms;

        private final boolean allPlatformsDisabled;
        private final boolean allPlatformsEnabled;

        @Nullable
        private final LocalDateTime lastStatusChange;
        @Nullable
        private final String lastStatusChangeAuthor;

        public PlatformsStatuses(Map<Platform, PlatformStatus> platforms) {
            this.platforms = Map.copyOf(platforms);

            allPlatformsDisabled = platforms.values().stream().allMatch(p -> p.getStatus().equals(Status.disabled));
            allPlatformsEnabled = platforms.values().stream().allMatch(p -> p.getStatus().equals(Status.enabled));

            lastStatusChangeAuthor = platforms.values().stream()
                    .reduce((p1, p2) -> (p1.getStatusChangeDateTime().isBefore(p2.getStatusChangeDateTime()) ? p2 : p1))
                    .map(PlatformStatus::getStatusChangeLogin)
                    .orElse(null);

            lastStatusChange = platforms.values().stream()
                    .reduce((p1, p2) -> (p1.getStatusChangeDateTime().isBefore(p2.getStatusChangeDateTime()) ? p2 : p1))
                    .map(PlatformStatus::getStatusChangeDateTime)
                    .orElse(null);
        }

        public boolean isAllPlatformsDisabled() {
            return allPlatformsDisabled;
        }

        public boolean isAllPlatformsDisabled(Collection<Platform> platforms) {
            return platforms.stream().allMatch(p -> this.platforms.get(p).getStatus().equals(Status.disabled));
        }

        public boolean isAnyPlatformDisabled(Collection<Platform> platforms) {
            return platforms.stream().anyMatch(p -> this.platforms.get(p).getStatus().equals(Status.disabled));
        }

        public boolean isAnyPlatformEnabled(Collection<Platform> platforms) {
            return platforms.stream().anyMatch(p -> this.platforms.get(p).getStatus().equals(Status.enabled));
        }

        public boolean isAllPlatformsEnabled() {
            return allPlatformsEnabled;
        }

        public Optional<LocalDateTime> getLastStatusChange() {
            return Optional.ofNullable(lastStatusChange);
        }

        public Optional<String> getLastStatusChangeAuthor() {
            return Optional.ofNullable(lastStatusChangeAuthor);
        }

        public boolean isPlatformEnabled(Platform platform) {
            return platforms.get(platform).getStatus().equals(Status.enabled);
        }

        public Map<Platform, PlatformStatus> getPlatforms() {
            return platforms;
        }
    }

    @Value
    public static class PlatformsStatusesResponse {
        List<PlatformStatus> platforms;
    }

    @Data
    public static class PlatformStatus {
        @JsonProperty("status")
        Status status;
        @JsonProperty("status_change_login")
        String statusChangeLogin;
        @JsonProperty("name")
        String name;
        @JsonProperty("status_change_datetime")
        LocalDateTime statusChangeDateTime;
    }

    interface TestEnvApi {

        @GET("/api/te/v1.0/autocheck/precommit/service_level")
        PlatformsStatusesResponse getPrecommitServiceLevel();

        @GET("/handlers/{pathName}")
        Response<Void> handler(@Path("pathName") String pathName);
    }
}
