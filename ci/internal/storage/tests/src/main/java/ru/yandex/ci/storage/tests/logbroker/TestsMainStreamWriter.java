package ru.yandex.ci.storage.tests.logbroker;

import java.util.List;

import lombok.AllArgsConstructor;

import ru.yandex.ci.storage.core.MainStreamMessages;

@AllArgsConstructor
public class TestsMainStreamWriter extends TestsMessageWriter {
    private final TestLogbrokerService testLogbrokerService;

    public void write(List<MainStreamMessages.MainStreamMessage> messages) {
        this.testLogbrokerService.writeToMainStream(messages);
    }
}
