package ru.yandex.ci.core.logbroker;

import java.util.function.Supplier;

import ru.yandex.kikimr.persqueue.auth.Credentials;

public interface LogbrokerCredentialsProvider extends Supplier<Credentials> {
}
