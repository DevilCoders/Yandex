package ru.yandex.ci.core.discovery;

import java.util.Objects;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true)
@Table(name = "main/CommitDiscoveryProgress")
@GlobalIndex(name = CommitDiscoveryProgress.IDX_BY_DIR_DISCOVERY_FINISHED_FOR_PARENTS,
        fields = "dirDiscoveryFinishedForParents")
@GlobalIndex(name = CommitDiscoveryProgress.IDX_BY_GRAPH_DISCOVERY_FINISHED_FOR_PARENTS,
        fields = "graphDiscoveryFinishedForParents")
@GlobalIndex(name = CommitDiscoveryProgress.IDX_BY_STORAGE_DISCOVERY_FINISHED_FOR_PARENTS,
        fields = "storageDiscoveryFinishedForParents")
@GlobalIndex(name = CommitDiscoveryProgress.IDX_BY_PCI_DSS_DISCOVERY_FINISHED_FOR_PARENTS,
        fields = "pciDssDiscoveryFinishedForParents")
@GlobalIndex(name = CommitDiscoveryProgress.IDX_BY_PCI_DSS_STATE,
        fields = "pciDssState")
public class CommitDiscoveryProgress implements Entity<CommitDiscoveryProgress> {

    public static final String IDX_BY_DIR_DISCOVERY_FINISHED_FOR_PARENTS =
            "IDX_BY_DIR_DISCOVERY_FINISHED_FOR_PARENTS";
    public static final String IDX_BY_GRAPH_DISCOVERY_FINISHED_FOR_PARENTS =
            "IDX_BY_GRAPH_DISCOVERY_FINISHED_FOR_PARENTS";
    public static final String IDX_BY_STORAGE_DISCOVERY_FINISHED_FOR_PARENTS =
            "IDX_BY_STORAGE_DISCOVERY_FINISHED_FOR_PARENTS";
    public static final String IDX_BY_PCI_DSS_DISCOVERY_FINISHED_FOR_PARENTS =
            "IDX_BY_PCI_DSS_DISCOVERY_FINISHED_FOR_PARENTS";
    public static final String IDX_BY_PCI_DSS_STATE =
            "IDX_BY_PCI_DSS_STATE";

    @Nonnull
    CommitDiscoveryProgress.Id id;

    @Nonnull
    @Column(flatten = false)
    OrderedArcRevision arcRevision;

    @With
    @Column
    boolean dirDiscoveryFinished;

    @With
    @Column
    boolean dirDiscoveryFinishedForParents;

    @With
    @Column
    boolean graphDiscoveryFinished;

    @With
    @Column
    boolean graphDiscoveryFinishedForParents;


    @With
    @Column
    Boolean storageDiscoveryFinished;

    @With
    @Column
    Boolean storageDiscoveryFinishedForParents;

    @With
    @Column(dbType = DbType.STRING)
    PciDssState pciDssState;

    @With
    @Column
    Boolean pciDssDiscoveryFinishedForParents;

    @Nonnull
    @Override
    public Id getId() {
        return id;
    }

    public static CommitDiscoveryProgress of(OrderedArcRevision arcRevision) {
        return CommitDiscoveryProgress.builder()
                .arcRevision(arcRevision)
                .build();
    }

    public boolean isStorageDiscoveryFinished() {
        return Objects.requireNonNullElse(storageDiscoveryFinished, Boolean.FALSE);
    }

    public boolean isStorageDiscoveryFinishedForParents() {
        return Objects.requireNonNullElse(storageDiscoveryFinishedForParents, Boolean.FALSE);
    }

    public boolean isPciDssStateProcessed() {
        return Objects.equals(
                PciDssState.PROCESSED,
                Objects.requireNonNullElse(pciDssState, PciDssState.NOT_PROCESSED)
        );
    }

    public boolean getPciDssDiscoveryFinishedForParents() {
        return Objects.requireNonNullElse(pciDssDiscoveryFinishedForParents, Boolean.FALSE);
    }

    public boolean isDiscoveryFinished(DiscoveryType discoveryType) {
        return switch (discoveryType) {
            case DIR -> isDirDiscoveryFinished();
            case GRAPH -> isGraphDiscoveryFinished();
            case STORAGE -> isStorageDiscoveryFinished();
            case PCI_DSS -> isPciDssStateProcessed();
        };
    }

    public boolean isDiscoveryFinishedForParents(DiscoveryType discoveryType) {
        return switch (discoveryType) {
            case DIR -> isDirDiscoveryFinishedForParents();
            case GRAPH -> isGraphDiscoveryFinishedForParents();
            case STORAGE -> isStorageDiscoveryFinishedForParents();
            case PCI_DSS -> getPciDssDiscoveryFinishedForParents();
        };
    }

    public CommitDiscoveryProgress withDiscoveryFinished(DiscoveryType discoveryType) {
        return switch (discoveryType) {
            case DIR -> withDirDiscoveryFinished(true);
            case GRAPH -> withGraphDiscoveryFinished(true);
            case STORAGE -> withStorageDiscoveryFinished(true);
            case PCI_DSS -> withPciDssState(PciDssState.PROCESSED);
        };
    }

    public CommitDiscoveryProgress withDiscoveryFinishedForParents(DiscoveryType discoveryType) {
        return switch (discoveryType) {
            case DIR -> withDirDiscoveryFinishedForParents(true);
            case GRAPH -> withGraphDiscoveryFinishedForParents(true);
            case STORAGE -> withStorageDiscoveryFinishedForParents(true);
            case PCI_DSS -> withPciDssDiscoveryFinishedForParents(true);
        };
    }

    @Persisted
    public enum PciDssState {
        NOT_PROCESSED,
        PROCESSING,
        PROCESSED
    }

    public static class Builder {

        {
            storageDiscoveryFinished = false;
            storageDiscoveryFinishedForParents = false;
            pciDssState = PciDssState.NOT_PROCESSED;
            pciDssDiscoveryFinishedForParents = false;
        }

        private Builder id(CommitDiscoveryProgress.Id id) {
            /* The method is used inside `CommitDiscoveryProgress.toBuilder`.
               The method is private cause we want to keep the field in sync with `arcRevision` and
               we want to allow a user to change this field only via `arcRevision` */
            this.id = id;
            return this;
        }

        public Builder arcRevision(OrderedArcRevision arcRevision) {
            this.arcRevision = arcRevision;
            this.id = new Id(arcRevision.getCommitId());
            return this;
        }
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<CommitDiscoveryProgress> {
        @Column(dbType = DbType.UTF8)
        @Nonnull
        String commitId;

        public static CommitDiscoveryProgress.Id of(ArcRevision arcRevision) {
            return new Id(arcRevision.getCommitId());
        }
    }

}
