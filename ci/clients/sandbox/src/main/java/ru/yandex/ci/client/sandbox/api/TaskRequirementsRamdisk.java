package ru.yandex.ci.client.sandbox.api;

import org.apache.commons.lang3.builder.ReflectionToStringBuilder;
import org.apache.commons.lang3.builder.ToStringStyle;

/**
 * @author Alexander Kramarev (pochemuto@yandex-team.ru)
 * @since 14.09.2020
 */
public class TaskRequirementsRamdisk {
    private static final String TMPFS = "tmpfs";

    private long size;
    private String type = TMPFS;

    public long getSize() {
        return size;
    }

    public TaskRequirementsRamdisk setSize(long size) {
        this.size = size;
        return this;
    }

    public String getType() {
        return type;
    }

    public TaskRequirementsRamdisk setType(String type) {
        this.type = type;
        return this;
    }

    @Override
    public String toString() {
        return ReflectionToStringBuilder.toString(this, ToStringStyle.SHORT_PREFIX_STYLE);
    }
}
