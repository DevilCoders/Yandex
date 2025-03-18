package ru.yandex.ci.client.tvm;

import okhttp3.Request;

import ru.yandex.ci.client.base.http.AuthProvider;
import ru.yandex.passport.tvmauth.TvmClient;

public class TvmAuthProvider implements AuthProvider {
    private final TvmClient tvmClient;
    private final int destTvmClientId;

    public TvmAuthProvider(TvmClient tvmClient, int destTvmClientId) {
        this.tvmClient = tvmClient;
        this.destTvmClientId = destTvmClientId;
    }

    @Override
    public void addAuth(Request.Builder request) {
        request.header(TvmHeaders.SERVICE_TICKET, getServiceTicket());
    }

    public String getServiceTicket() {
        return tvmClient.getServiceTicketFor(destTvmClientId);
    }
}
