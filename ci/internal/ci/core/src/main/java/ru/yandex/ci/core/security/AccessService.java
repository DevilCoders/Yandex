package ru.yandex.ci.core.security;

import java.util.Set;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.abc.AbcService;

@Slf4j
@RequiredArgsConstructor
public class AccessService {
    @Nonnull
    private final AbcService abcService;
    @Nonnull
    private final String adminAbcGroup;
    @Nonnull
    private final String adminAbcScope;

    public void checkAccess(String login, String project) {
        if (hasAccessOrAdmin(login, project)) {
            return;
        }
        throw GrpcUtils.permissionDeniedException("User [%s] is not in abc service [%s]".formatted(login, project));
    }

    public void checkAccess(String login, String project, Set<String> scopes, Set<String> roles, Set<String> duties) {
        if (hasAccessOrAdmin(login, project, scopes, roles, duties)) {
            return;
        }
        throw GrpcUtils.permissionDeniedException(
                String.format("User [%s] is not in abc service [%s] with scopes %s, roles %s or duties %s",
                        login, project, scopes, roles, duties));
    }

    boolean hasAccessOrAdmin(String login, String project) {
        boolean access = abcService.isMember(login, project) || isCiAdmin(login);
        log.info("Access {} for user = {}, project = {}",
                access ? "granted" : "denied", login, project);
        return access;
    }

    boolean hasAccessOrAdmin(String login, String project, Set<String> scopes, Set<String> roles, Set<String> duties) {
        boolean access = abcService.isMember(login, project, scopes, roles, duties) || isCiAdmin(login);
        log.info("Access {} for user = {}, project = {}, scopes = {}",
                access ? "granted" : "denied", login, project, scopes);
        return access;
    }

    public boolean hasAccess(String login, String project, Set<String> scopes) {
        return abcService.isMember(login, project, scopes);
    }

    public boolean isCiAdmin(String login) {
        return abcService.isMember(login, adminAbcGroup, Set.of(adminAbcScope));
    }

}
