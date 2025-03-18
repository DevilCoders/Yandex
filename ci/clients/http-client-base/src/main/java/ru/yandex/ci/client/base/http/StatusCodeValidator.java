package ru.yandex.ci.client.base.http;

public interface StatusCodeValidator {

    boolean validate(int code);
}
