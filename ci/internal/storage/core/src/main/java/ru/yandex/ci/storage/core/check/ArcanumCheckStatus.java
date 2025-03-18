package ru.yandex.ci.storage.core.check;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto.Status;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Persisted
@Value
@BenderBindAllFields
public class ArcanumCheckStatus {

    private static final ArcanumCheckStatus SUCCESS = new ArcanumCheckStatus(Status.SUCCESS, null);
    private static final ArcanumCheckStatus CANCELLED = new ArcanumCheckStatus(Status.CANCELLED, null);
    private static final ArcanumCheckStatus ERROR = new ArcanumCheckStatus(Status.ERROR, null);
    private static final ArcanumCheckStatus PENDING = new ArcanumCheckStatus(Status.PENDING, null);
    private static final ArcanumCheckStatus SKIPPED = new ArcanumCheckStatus(Status.SKIPPED, null);

    private static final int ARCANUM_MAX_ERROR_LENGTH = 500;

    @Nonnull
    Status status;
    @Nullable
    String message;

    public static ArcanumCheckStatus error() {
        return ERROR;
    }

    public static ArcanumCheckStatus cancelled() {
        return CANCELLED;
    }

    public static ArcanumCheckStatus success() {
        return SUCCESS;
    }

    public static ArcanumCheckStatus success(String message) {
        return new ArcanumCheckStatus(Status.SUCCESS, arcanumMessage(message));
    }

    public static ArcanumCheckStatus pending() {
        return PENDING;
    }

    public static ArcanumCheckStatus skipped() {
        return SKIPPED;
    }

    public static ArcanumCheckStatus failure(String message) {
        return new ArcanumCheckStatus(Status.FAILURE, arcanumMessage(message));
    }

    public static ArcanumCheckStatus error(String message) {
        return new ArcanumCheckStatus(Status.ERROR, arcanumMessage(message));
    }

    public static ArcanumCheckStatus skipped(String message) {
        return new ArcanumCheckStatus(Status.SKIPPED, arcanumMessage(message));
    }

    private static String arcanumMessage(String message) {
        return StringUtils.abbreviate(message, ARCANUM_MAX_ERROR_LENGTH);
    }
}
