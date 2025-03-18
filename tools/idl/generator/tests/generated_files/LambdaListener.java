package ru.test.docs;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;

public interface LambdaListener {
    @UiThread
    public void onResponse(
        @NonNull Response response);

    @UiThread
    public void onError(
        @NonNull TestError error);
}
