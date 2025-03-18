package ru.yandex.ci.core.timeline;

import java.time.Instant;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcBranch;

@Value
@Table(name = "main/BranchItemByUpdateDate")
public class TimelineBranchItemByUpdateDate implements Entity<TimelineBranchItemByUpdateDate> {
    public static final String PROCESS_ID_FIELD = "id.processId";
    public static final String UPDATE_DATE_FIELD = "id.updateDate";
    public static final String BRANCH_NAME_FIELD = "id.branch";

    Id id;

    @Override
    public Id getId() {
        return id;
    }

    public static TimelineBranchItemByUpdateDate of(TimelineBranchItem item) {
        return new TimelineBranchItemByUpdateDate(
                Id.of(item.getId().getProcessId(), item.getVcsInfo().getUpdatedDate(), item.getId().getBranch())
        );
    }

    public TimelineBranchItem.Id getItemId() {
        return TimelineBranchItem.Id.of(id.getProcessId(), id.getBranch());
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<TimelineBranchItemByUpdateDate> {
        @Column(name = "idx_processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "idx_updateDate")
        Instant updateDate;

        @Column(name = "idx_branchName", dbType = DbType.UTF8)
        String branch;
    }

    @Value
    public static class Offset {
        Instant updateDate;
        ArcBranch branch;

        public static Offset of(TimelineBranchItem item) {
            return new Offset(
                    item.getVcsInfo().getUpdatedDate(),
                    ArcBranch.ofBranchName(item.getId().getBranch())
            );
        }
    }
}
