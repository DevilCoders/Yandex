package ru.yandex.ci.client.base.http;

import java.util.Arrays;
import java.util.Set;
import java.util.stream.Collectors;

public class StatusCodeValidators {

    private StatusCodeValidators() {
    }

    public static Http2xxStatusCodeValidator http2xxStatusCodeValidator() {
        return Http2xxStatusCodeValidator.INSTANCE;
    }

    public static StatusCodeValidator forCodes(int... allowedCodes) {
        Set<Integer> codes = Arrays.stream(allowedCodes).boxed().collect(Collectors.toSet());
        return codes::contains;
    }

    private static class Http2xxStatusCodeValidator implements StatusCodeValidator {
        private static final Http2xxStatusCodeValidator INSTANCE = new Http2xxStatusCodeValidator();

        @Override
        public boolean validate(int code) {
            return code >= 200 && code < 300;
        }
    }
}
