package ru.yandex.ci.client.juggler;

import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.POST;

import ru.yandex.ci.client.base.http.Idempotent;
import ru.yandex.ci.client.juggler.model.ChecksStatusRequest;
import ru.yandex.ci.client.juggler.model.ChecksStatusResponse;
import ru.yandex.ci.client.juggler.model.MuteCreateRequest;
import ru.yandex.ci.client.juggler.model.MuteRemoveRequest;
import ru.yandex.ci.client.juggler.model.MutesRequest;
import ru.yandex.ci.client.juggler.model.MutesResponse;

interface JugglerApi {
    @Idempotent
    @POST("/v2/checks/get_checks_state")
    ChecksStatusResponse getCheckState(@Body ChecksStatusRequest request);

    @Idempotent
    @POST("/v2/mutes/get_mutes")
    MutesResponse getMutes(@Body MutesRequest request);

    @POST("/v2/mutes/set_mutes")
    Response<Void> setMutes(@Body MuteCreateRequest request);

    @POST("/v2/mutes/remove_mutes")
    Response<Void> removeMutes(@Body MuteRemoveRequest request);
}
