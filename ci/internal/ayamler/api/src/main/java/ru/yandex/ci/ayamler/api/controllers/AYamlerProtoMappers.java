package ru.yandex.ci.ayamler.api.controllers;

import java.util.Optional;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;

import ru.yandex.ci.ayamler.AYaml;
import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.project.AutocheckProject;

public class AYamlerProtoMappers {
    private AYamlerProtoMappers() {
    }

    static Ayamler.StrongMode toProtoStrongMode(
            String path, ArcRevision revision, AYaml aYaml, @Nullable String login
    ) {
        Preconditions.checkState(aYaml.getStrongMode() != null,
                "strongMode in aYaml %s cannot be null, expect patched version", aYaml.getPath());
        var strongModeStatus = toProtoStrongModeStatus(aYaml.getStrongMode().isEnabled());
        return toProtoStrongMode(path, revision, strongModeStatus, login, aYaml.isOwner())
                .setAyaml(toProtoAYaml(aYaml))
                .build();
    }

    static Ayamler.StrongModeStatus toProtoStrongModeStatus(boolean strongMode) {
        return strongMode ? Ayamler.StrongModeStatus.ON : Ayamler.StrongModeStatus.OFF;
    }

    static Ayamler.StrongMode.Builder toProtoStrongMode(
            String path, ArcRevision revision, Ayamler.StrongModeStatus off, @Nullable String login, boolean isOwner
    ) {
        var builder = Ayamler.StrongMode.newBuilder()
                .setRevision(revision.getCommitId())
                .setPath(path)
                .setStatus(off)
                .setAyaml(
                        Ayamler.AYaml.newBuilder()
                                .setService(AutocheckProject.NAME)
                                .build()
                )
                .setIsOwner(isOwner);
        if (login != null) {
            builder.setLogin(login);
        }
        return builder;
    }

    static Ayamler.AYaml toProtoAYaml(AYaml aYaml) {
        var builder = Ayamler.AYaml.newBuilder()
                .setPath(aYaml.getPath().toString())
                .setValid(aYaml.isValid())
                .setService(aYaml.getService() == null ? AutocheckProject.NAME : aYaml.getService());

        Optional.ofNullable(aYaml.getErrorMessage())
                .ifPresent(builder::setErrorMessage);

        return builder.build();
    }

    static Ayamler.AbcService toProtoAbcService(String path, ArcRevision revision, @Nullable AYaml aYaml) {
        var builder = Ayamler.AbcService.newBuilder()
                .setPath(path)
                .setRevision(revision.getCommitId());
        if (aYaml != null) {
            builder.setAyaml(toProtoAYaml(aYaml));
        }
        if (aYaml != null && aYaml.getService() != null) {
            builder.setSlug(aYaml.getService());
        }
        return builder.build();
    }
}
