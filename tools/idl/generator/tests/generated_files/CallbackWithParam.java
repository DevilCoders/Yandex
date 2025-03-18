package ru.test.docs;

import androidx.annotation.UiThread;

public interface CallbackWithParam {
    @UiThread
    public void onCallback(
        int i);
}
