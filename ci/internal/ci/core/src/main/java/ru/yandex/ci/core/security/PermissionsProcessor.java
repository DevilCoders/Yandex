package ru.yandex.ci.core.security;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.config.a.model.Permissions;

public class PermissionsProcessor {

    private PermissionsProcessor() {
        //
    }

    public static Permissions overridePermissions(@Nullable Permissions parent, @Nullable Permissions child) {
        if (parent == null) {
            return Objects.requireNonNullElse(child, Permissions.EMPTY);
        } else if (child == null) {
            return Objects.requireNonNullElse(parent, Permissions.EMPTY);
        }

        var allScopes = PermissionScope.values();
        var map = new LinkedHashMap<PermissionScope, List<PermissionRule>>(allScopes.length);
        for (var scope : allScopes) {
            var parentList = parent.getPermissions(scope);
            var childList = child.getPermissions(scope);
            var list = childList.isEmpty() ? parentList : childList;
            if (!list.isEmpty()) {
                map.put(scope, list);
            }
        }

        if (child.getDefaultPermissionsForOwner() != null) {
            parent = parent.withDefaultPermissionsForOwner(child.getDefaultPermissionsForOwner());
        }

        return parent.withPermissions(map);
    }

    public static Permissions finalizePermissions(Permissions permissions) {
        var src = permissions.getPermissions();
        var target = new LinkedHashMap<>(src);
        for (var e : src.entrySet()) {
            target.put(e.getKey(), compressRules(e.getValue()));
        }
        return permissions.withPermissions(target);
    }

    public static List<PermissionRule> compressRules(@Nullable List<PermissionRule> rules) {
        if (rules == null || rules.isEmpty()) {
            return List.of();
        }

        // Make sure we keep rule allowing access to all users
        var scopes = new LinkedHashMap<String, PermissionRuleHolder>(rules.size());
        for (var rule : rules) {
            var service = rule.getAbcService();
            var abcScopes = rule.getAbcScopes();
            var abcRoles = rule.getAbcRoles();
            var abcDuties = rule.getAbcDuties();
            if (abcScopes.isEmpty() && abcRoles.isEmpty() && abcDuties.isEmpty()) {
                scopes.put(service, new PermissionRuleHolder());
            } else {
                var oldHolder = scopes.get(service);
                if (oldHolder != null) {
                    if (!oldHolder.anyRole) {
                        oldHolder.abcScopes.addAll(abcScopes);
                        oldHolder.abcRoles.addAll(abcRoles);
                        oldHolder.abcDuties.addAll(abcDuties);
                    }
                } else {
                    var newHolder = new PermissionRuleHolder();
                    newHolder.anyRole = false;
                    newHolder.abcScopes.addAll(abcScopes);
                    newHolder.abcRoles.addAll(abcRoles);
                    newHolder.abcDuties.addAll(abcDuties);
                    scopes.put(service, newHolder);
                }
            }
        }

        // Return compressed rules
        var result = new ArrayList<PermissionRule>(scopes.size());
        for (var e : scopes.entrySet()) {
            var service = e.getKey();
            var holder = e.getValue();
            result.add(PermissionRule.of(
                    service,
                    List.copyOf(holder.abcScopes),
                    List.copyOf(holder.abcRoles),
                    List.copyOf(holder.abcDuties))
            );
        }
        return result;
    }

    private static class PermissionRuleHolder {
        boolean anyRole = true;
        Set<String> abcScopes = new LinkedHashSet<>();
        Set<String> abcRoles = new LinkedHashSet<>();
        Set<String> abcDuties = new LinkedHashSet<>();
    }
}
