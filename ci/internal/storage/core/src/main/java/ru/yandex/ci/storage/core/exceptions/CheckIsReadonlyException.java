package ru.yandex.ci.storage.core.exceptions;

import ru.yandex.ci.core.exceptions.CiFailedPreconditionException;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

public class CheckIsReadonlyException extends CiFailedPreconditionException {
    public CheckIsReadonlyException(CheckEntity.Id checkId, String message) {
        super(message + ", check is readonly: " + checkId);
    }
}
