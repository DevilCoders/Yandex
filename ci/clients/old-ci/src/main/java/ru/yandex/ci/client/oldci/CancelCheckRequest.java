package ru.yandex.ci.client.oldci;

import lombok.Value;

@Value
public class CancelCheckRequest {
    String action = "cancel";
}
