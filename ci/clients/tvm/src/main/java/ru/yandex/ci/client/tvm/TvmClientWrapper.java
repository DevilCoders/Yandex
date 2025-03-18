package ru.yandex.ci.client.tvm;

import com.google.common.base.Strings;

import ru.yandex.passport.tvmauth.BlackboxEnv;
import ru.yandex.passport.tvmauth.NativeTvmClient;
import ru.yandex.passport.tvmauth.TvmApiSettings;
import ru.yandex.passport.tvmauth.TvmClient;

public class TvmClientWrapper {

    private TvmClientWrapper() {
    }

    public static TvmClient getTvmClient(
            int selfTvmClientId,
            String selfTvmSecret,
            int[] targetClientIds,
            String blackboxEnv
    ) {
        TvmApiSettings tvmApiSettings = new TvmApiSettings().setSelfTvmId(selfTvmClientId)
                .enableServiceTicketChecking();
        if (!Strings.isNullOrEmpty(selfTvmSecret) && targetClientIds.length > 0) {
            tvmApiSettings.enableServiceTicketsFetchOptions(selfTvmSecret, targetClientIds);
        }
        if (!Strings.isNullOrEmpty(blackboxEnv)) {
            tvmApiSettings.enableUserTicketChecking(BlackboxEnv.valueOf(blackboxEnv));
        }
        return new NativeTvmClient(tvmApiSettings);
    }


}
