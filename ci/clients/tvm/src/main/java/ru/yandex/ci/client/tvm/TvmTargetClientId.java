package ru.yandex.ci.client.tvm;

import ru.yandex.passport.tvmauth.TvmClient;

/**
 * {@link TvmClient} требует список целевых id, для которых будет выдаваться service-ticket.
 * Этот класс используется для независимого определения бинов, использующих {@link TvmClient}.
 * Определите бин этого типа с id сервиса, для которого вы будете получать service-ticket, и id
 * будет использован при инициализации {@link TvmClient}.
 *
 */
public class TvmTargetClientId {
    private final int id;

    public TvmTargetClientId(int id) {
        this.id = id;
    }

    public int getId() {
        return id;
    }
}
