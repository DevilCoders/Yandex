package ru.yandex.ci.core.security;

import java.util.Set;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.core.abc.AbcService;

import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class AccessServiceTest {
    private static final String ADMIN_GROUP = "ci";
    private static final String ADMIN_SCOPE = "admin";
    private static final String USER = "test_user";

    private AccessService accessService;

    @Mock
    private AbcService abcService;

    @BeforeEach
    public void setUp() {
        accessService = new AccessService(abcService, ADMIN_GROUP, ADMIN_SCOPE);
    }

    @Test
    void denyForNotMembers() {
        when(abcService.isMember(USER, "group")).thenReturn(false);

        assertThatThrownBy(() -> accessService.checkAccess(USER, "group"))
                .hasMessage("PERMISSION_DENIED: User [test_user] is not in abc service [group]");
    }

    @Test
    void allowForMembers() {
        when(abcService.isMember(USER, "group")).thenReturn(true);

        accessService.checkAccess(USER, "group");
    }

    @Test
    void allowForNotMembersButAdmins() {
        when(abcService.isMember(USER, "group")).thenReturn(false);
        when(abcService.isMember(USER, ADMIN_GROUP, Set.of(ADMIN_SCOPE)))
                .thenReturn(true);

        accessService.checkAccess(USER, "group");
    }
}
