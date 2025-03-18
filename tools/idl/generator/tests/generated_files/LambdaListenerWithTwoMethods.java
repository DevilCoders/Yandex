package ru.test.docs;

import androidx.annotation.UiThread;

public interface LambdaListenerWithTwoMethods {
    @UiThread
    public void onSuccess();

    @UiThread
    public void onError(
        int error);
}
