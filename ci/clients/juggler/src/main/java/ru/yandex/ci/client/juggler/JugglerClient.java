package ru.yandex.ci.client.juggler;

import java.util.List;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.juggler.model.ChecksStatusRequest;
import ru.yandex.ci.client.juggler.model.MuteCreateRequest;
import ru.yandex.ci.client.juggler.model.MuteInfoResponse;
import ru.yandex.ci.client.juggler.model.MuteRemoveRequest;
import ru.yandex.ci.client.juggler.model.MutesRequest;
import ru.yandex.ci.client.juggler.model.Status;
import ru.yandex.ci.client.juggler.model.StatusResponse;

@Slf4j
public class JugglerClient {
    private final JugglerApi api;

    private JugglerClient(HttpClientProperties httpClientProperties) {
        var objectMapper = RetrofitClient.Builder.defaultObjectMapper();
        objectMapper.registerModule(new JugglerModule());

        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .objectMapper(objectMapper)
                .build(JugglerApi.class);
    }

    public static JugglerClient create(HttpClientProperties httpClientProperties) {
        return new JugglerClient(httpClientProperties);
    }

    public List<Status> getChecksStatus(ChecksStatusRequest request) {
        if (request.getFilters().isEmpty()) {
            return List.of();
        }
        var response = api.getCheckState(request);
        log.info("Get checks status, request: {}, response: {}", request, response);
        return response.getStatuses().stream().map(StatusResponse::getStatus).collect(Collectors.toList());
    }

    public List<MuteInfoResponse> getMutes(MutesRequest request) {
        if (request.getFilters().isEmpty()) {
            return List.of();
        }
        var response = api.getMutes(request);
        log.info("Get mutes request: {}, response: {}", request, response);
        return response.getItems();
    }

    public void createMute(MuteCreateRequest request) {
        if (request.getFilters().isEmpty()) {
            return;
        }
        log.info("Mute request: {}", request);
        api.setMutes(request);
    }

    public void removeMutes(MuteRemoveRequest request) {
        if (request.getIds().isEmpty()) {
            return;
        }
        log.info("Remove mutes request: {}", request);
        api.removeMutes(request);
    }

}
