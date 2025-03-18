package ru.yandex.ci.client.base.http;

import javax.annotation.Nullable;

public interface CallsMonitor<Request, Response> {

    String getClientName();

    SingleCallMonitor<Response> start(Request request, int tryNum);

    interface SingleCallMonitor<Response> {
        void success(@Nullable Response response);

        void failure(Exception t);

        /**
         * Returns response as a string.
         * Make note - it will completely read response so it can't be used
         * for a normal response parsing.
         * Make sense after {@link #success(Object)} operation only
         *
         * @return cached response, usually during error handling
         */
        @Nullable
        String getCachedErrorResponse();
    }
}
