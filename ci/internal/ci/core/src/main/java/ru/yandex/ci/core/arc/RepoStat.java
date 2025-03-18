package ru.yandex.ci.core.arc;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

@Value
public class RepoStat {

    @Nonnull
    String name;
    @Nonnull
    EntryType type;
    // Size is zero for directories
    long size;
    boolean executable;
    boolean symlink;
    // Empty for dirs
    String fileOid;
    @Nullable
    ArcCommit lastChanged;
    boolean encrypted;

    public enum EntryType {
        FILE,
        DIR,
        NONE,
    }

    public boolean isDir() {
        return type == EntryType.DIR;
    }

}
