package ru.yandex.ci.storage.core.db.model.test_mute;

import java.util.UUID;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@Builder
@Table(name = "OldCiMuteAction")
public class OldCiMuteActionEntity implements Entity<OldCiMuteActionEntity> {
    Id id;
    String testId;
    String toolchain;
    boolean muted;
    String reason;

    public static OldCiMuteActionEntity of(TestMuteEntity action) {
        return OldCiMuteActionEntity.builder()
                .id(new Id(UUID.randomUUID().toString()))
                .testId(action.getOldTestId().isEmpty() ? action.getOldSuiteId() : action.getOldTestId())
                .toolchain(action.getId().getTestId().getToolchain())
                .muted(action.isMuted())
                .reason("%s Check link: https://a.yandex-team.ru/ci-card-preview/%s".formatted(
                        action.getReason(),
                        action.getIterationId().getCheckId()
                ))
                .build();
    }

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<OldCiMuteActionEntity> {
        String id;
    }

}
