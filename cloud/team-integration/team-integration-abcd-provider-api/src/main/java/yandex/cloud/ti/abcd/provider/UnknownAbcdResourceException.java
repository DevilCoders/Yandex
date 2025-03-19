package yandex.cloud.ti.abcd.provider;

import yandex.cloud.iam.exception.NotFoundException;

public class UnknownAbcdResourceException extends NotFoundException {

    UnknownAbcdResourceException(String message) {
        super(message);
    }

}
