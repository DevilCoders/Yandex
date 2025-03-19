package yandex.cloud.ti.abcd.adapter;


import yandex.cloud.iam.exception.UnavailableException;

public class AbcdAccountNotReadyException extends UnavailableException {

    public AbcdAccountNotReadyException(String internal) {
        super(internal);
    }

}
