package ru.yandex.ci.storage.core.db.model.check;

import java.time.Instant;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;
import lombok.With;
import org.apache.commons.lang3.StringUtils;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.storage.core.CheckOuterClass.ArchiveState;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true, buildMethodName = "buildInternal")
@With
@Table(name = "Checks")
@GlobalIndex(name = CheckEntity.IDX_BY_REVISIONS, fields = {"left.revision", "right.revision"})
@GlobalIndex(name = CheckEntity.IDX_BY_RIGHT_REVISION, fields = {"right.revision"})
@GlobalIndex(name = CheckEntity.IDX_BY_STATUS_AND_CREATED, fields = {"status", "created"})
@GlobalIndex(name = CheckEntity.IDX_BY_ARCHIVE_STATE_AND_CREATED, fields = {"archiveState", "created"})
@GlobalIndex(name = CheckEntity.IDX_BY_PULL_REQUEST_ID_AND_CREATED, fields = {"pullRequestId", "created"})
public class CheckEntity implements Entity<CheckEntity> {
    public static final Long ID_START = 100000000000L;
    public static final int NUMBER_OF_ID_PARTITIONS = 999;

    public static final String IDX_BY_REVISIONS = "IDX_BY_REVISIONS";
    public static final String IDX_BY_RIGHT_REVISION = "IDX_BY_RIGHT_REVISION";
    public static final String IDX_BY_STATUS_AND_CREATED = "IDX_BY_STATUS_AND_CREATED";

    public static final String IDX_BY_ARCHIVE_STATE_AND_CREATED = "IDX_BY_ARCHIVE_STATE_AND_CREATED";

    public static final String IDX_BY_PULL_REQUEST_ID_AND_CREATED = "IDX_BY_PULL_REQUEST_ID_AND_CREATED";

    CheckEntity.Id id;

    CheckType type;

    CheckStatus status;

    @Nullable
    ArchiveState archiveState;

    Set<String> tags;

    Long diffSetId;

    StorageRevision left;

    StorageRevision right;

    String author;

    int shardOutPartition;

    @Nullable
    @Column(flatten = false)
    ShardingSettings shardingSettings;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Column(dbType = DbType.TIMESTAMP)
    @Nullable
    Instant completed;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant archived;

    @Column(flatten = false, dbType = DbType.JSON)
    @Nullable
    List<LargeAutostart> autostartLargeTests;

    @Column(flatten = false, dbType = DbType.JSON)
    @Nullable
    List<NativeBuild> nativeBuilds;

    @Column(flatten = false, dbType = DbType.JSON)
    @Nullable
    LargeTestsConfig largeTestsConfig;

    @Nullable
    Boolean runLargeTestsAfterDiscovery;

    @Nullable
    String runLargeTestsAfterDiscoveryBy;

    @Nullable
    Boolean reportStatusToArcanum;

    @Nullable
    Boolean testRestartsAllowed;

    @Nullable
    Boolean readOnly;

    @Nullable
    String autocheckConfigLeftRevision;

    @Nullable
    String autocheckConfigRightRevision;

    @Nullable
    Map<Common.StorageAttribute, String> attributes;

    @Column(flatten = false, dbType = DbType.JSON)
    @Nullable
    DistbuildPriority distbuildPriority;

    @Column(flatten = false, dbType = DbType.JSON)
    @Nullable
    Zipatch zipatch;

    @Nullable
    String testenvId; // For manual TE start link

    @Nullable
    Long pullRequestId;

    @Column(dbType = DbType.TIMESTAMP)
    @Nullable
    Instant diffSetEventCreated;

    @Nullable
    Collection<SuspiciousAlert> suspiciousAlerts;

    @Override
    public CheckEntity.Id getId() {
        return id;
    }

    public List<LargeAutostart> getAutostartLargeTests() {
        return Objects.requireNonNullElse(autostartLargeTests, List.of());
    }

    public List<NativeBuild> getNativeBuilds() {
        return Objects.requireNonNullElse(nativeBuilds, List.of());
    }

    public Boolean getTestRestartsAllowed() {
        return Objects.requireNonNullElse(testRestartsAllowed, false);
    }

    public String getAutocheckConfigLeftRevision() {
        return Objects.requireNonNullElse(autocheckConfigLeftRevision, "");
    }

    public String getAutocheckConfigRightRevision() {
        return Objects.requireNonNullElse(autocheckConfigRightRevision, "");
    }

    public boolean getRunLargeTestsAfterDiscovery() {
        return Objects.requireNonNullElse(runLargeTestsAfterDiscovery, false);
    }

    public boolean getReportStatusToArcanum() {
        return Objects.requireNonNullElse(reportStatusToArcanum, false);
    }

    public Collection<SuspiciousAlert> getSuspiciousAlerts() {
        return suspiciousAlerts == null ? List.of() : suspiciousAlerts;
    }

    public Map<Common.StorageAttribute, String> getAttributes() {
        return attributes == null ? Map.of() : attributes;
    }

    public CheckEntity run() {
        return this.toBuilder()
                .status(CheckStatus.RUNNING)
                .build();
    }

    public CheckEntity complete(CheckStatus status) {
        return complete(status, false);
    }

    public CheckEntity complete(CheckStatus status, boolean readOnly) {
        return this.toBuilder()
                .status(status)
                .completed(Instant.now())
                .readOnly(readOnly)
                .build();
    }

    public CheckEntity setAttribute(Common.StorageAttribute attribute, String value) {
        var currentAttributes = new HashMap<>(this.getAttributes());
        currentAttributes.put(attribute, value);

        return this.toBuilder()
                .attributes(currentAttributes)
                .build();
    }

    public String getAttributeOrDefault(Common.StorageAttribute attribute, String defaultValue) {
        return getAttributes().getOrDefault(attribute, defaultValue);
    }

    public boolean isFromEnvironment(String environment) {
        return this.getAttributeOrDefault(Common.StorageAttribute.SA_ENVIRONMENT, environment).equals(environment);
    }

    public String getEnvironment() {
        return this.getAttributeOrDefault(Common.StorageAttribute.SA_ENVIRONMENT, "");
    }

    public boolean isNotificationsDisabled() {
        var value = this.getAttributeOrDefault(Common.StorageAttribute.SA_NOTIFICATIONS_DISABLED, "false");
        return Boolean.parseBoolean(value);
    }

    public boolean isStressTest() {
        var value = this.getAttributeOrDefault(Common.StorageAttribute.SA_STRESS_TEST, "false");
        return Boolean.parseBoolean(value);
    }

    public boolean isReadOnly() {
        return Objects.requireNonNullElse(readOnly, false);
    }

    public boolean isFirstFailSent() {
        return Boolean.parseBoolean(this.getAttributeOrDefault(Common.StorageAttribute.SA_FIRST_FAIL_SENT, "false"));
    }

    public Optional<Long> getPullRequestId() {
        var branch = ArcBranch.ofString(right.getBranch());
        if (branch.isPr()) {
            return Optional.of(branch.getPullRequestId());
        }

        return Optional.empty();
    }

    public ArchiveState getArchiveState() {
        return Objects.requireNonNullElse(archiveState, ArchiveState.AS_NONE);
    }

    public String getTestenvId() {
        return testenvId == null ? "" : testenvId;
    }

    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<CheckEntity> {

        Long id;

        @Override
        public String toString() {
            return id.toString();
        }

        public static Id of(Long value) {
            return new Id(value);
        }

        public static Id of(String value) {
            if (StringUtils.isEmpty(value)) {
                throw new IllegalArgumentException("Check id is empty");
            }
            try {
                return new Id(Long.parseLong(value));
            } catch (NumberFormatException numberFormatException) {
                throw new IllegalArgumentException("Unable to parse check id as number: " + value);
            }
        }

        public int distribute(int numberOfQueues) {
            if (numberOfQueues <= 0) {
                return 0;
            }
            return (int) ((id / ID_START) % numberOfQueues);
        }

        public boolean sampleOut(ShardingSettings shardingSettings) {
            long checksToSkip = shardingSettings.getNumberOfChecksToSkip();
            return checksToSkip > 0 && (id / ID_START) % (checksToSkip + 1) != 1;
        }

    }

    public static class Builder {

        public CheckEntity build() {
            Preconditions.checkNotNull(id, "id is null");
            Preconditions.checkNotNull(left, "left is null");
            Preconditions.checkNotNull(right, "right is null");

            if (Objects.isNull(status)) {
                status = CheckStatus.CREATED;
            }

            if (Objects.isNull(archiveState)) {
                archiveState = ArchiveState.AS_NONE;
            }

            if (Objects.isNull(tags)) {
                tags = Set.of();
            }

            if (Objects.isNull(created)) {
                created = Instant.now();
            }

            if (reportStatusToArcanum == null) {
                reportStatusToArcanum = false;
            }

            if (testRestartsAllowed == null) {
                testRestartsAllowed = false;
            }

            var rightBranch = ArcBranch.ofString(right.getBranch());
            if (rightBranch.isPr()) {
                pullRequestId = rightBranch.getPullRequestId();
            }

            return buildInternal();
        }

    }

}
