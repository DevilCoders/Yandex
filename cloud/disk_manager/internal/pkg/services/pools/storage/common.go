package storage

import (
	"context"
	"fmt"
	"time"

	"github.com/gofrs/uuid"
	ydb_result "github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	pools_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func min(x, y uint64) uint64 {
	if x > y {
		return y
	}
	return x
}

func max(x, y uint64) uint64 {
	if y > x {
		return y
	}
	return x
}

func divideWithRoundingUp(x uint64, divisor uint64) uint64 {
	res := x / divisor

	if x%divisor != 0 {
		// Round up.
		return res + 1
	}

	return res
}

////////////////////////////////////////////////////////////////////////////////

func generateDiskID() string {
	return uuid.Must(uuid.NewV4()).String()
}

////////////////////////////////////////////////////////////////////////////////

type baseDiskStatus uint32

func (s *baseDiskStatus) UnmarshalYDB(res ydb_types.RawValue) error {
	*s = baseDiskStatus(res.Int64())
	return nil
}

// NOTE: These values are stored in DB, do not shuffle them around.
const (
	baseDiskStatusScheduling baseDiskStatus = iota
	baseDiskStatusCreating   baseDiskStatus = iota
	baseDiskStatusReady      baseDiskStatus = iota
	baseDiskStatusDeleting   baseDiskStatus = iota
	baseDiskStatusDeleted    baseDiskStatus = iota
)

func baseDiskStatusToString(status baseDiskStatus) string {
	switch status {
	case baseDiskStatusScheduling:
		return "scheduling"
	case baseDiskStatusCreating:
		return "creating"
	case baseDiskStatusReady:
		return "ready"
	case baseDiskStatusDeleting:
		return "deleting"
	case baseDiskStatusDeleted:
		return "deleted"
	}

	return fmt.Sprintf("unknown_%v", status)
}

////////////////////////////////////////////////////////////////////////////////

type baseDisk struct {
	id                  string
	imageID             string
	zoneID              string
	checkpointID        string
	createTaskID        string
	imageSize           uint64
	size                uint64
	srcDiskZoneID       string
	srcDiskID           string
	srcDiskCheckpointID string

	activeSlots    uint64
	maxActiveSlots uint64
	activeUnits    uint64
	units          uint64
	fromPool       bool
	retiring       bool
	deletedAt      time.Time
	status         baseDiskStatus
}

func (d *baseDisk) toBaseDisk() BaseDisk {
	var srcDisk *types.Disk
	if len(d.srcDiskID) != 0 {
		srcDisk = &types.Disk{
			ZoneId: d.srcDiskZoneID,
			DiskId: d.srcDiskID,
		}
	}

	return BaseDisk{
		ID:      d.id,
		ImageID: d.imageID,
		ZoneID:  d.zoneID,

		SrcDisk:             srcDisk,
		SrcDiskCheckpointID: d.srcDiskCheckpointID,
		CheckpointID:        d.checkpointID,
		CreateTaskID:        d.createTaskID,
		Size:                d.size,
		Ready:               d.status == baseDiskStatusReady,
	}
}

func (d *baseDisk) isInflight() bool {
	return d.status == baseDiskStatusScheduling || d.status == baseDiskStatusCreating
}

func (d *baseDisk) isDoomed() bool {
	return d.status >= baseDiskStatusDeleting
}

func (d *baseDisk) applyInvariants() {
	if !d.isDoomed() && !d.fromPool && d.activeSlots == 0 {
		d.status = baseDiskStatusDeleting
	}
}

func (d *baseDisk) freeSlots() uint64 {
	if d.isDoomed() || !d.fromPool {
		return 0
	}

	if d.units != 0 && d.activeUnits >= d.units {
		return 0
	}

	if d.maxActiveSlots < d.activeSlots {
		return 0
	}

	return d.maxActiveSlots - d.activeSlots
}

func (d *baseDisk) freeUnits() uint64 {
	slots := d.freeSlots()

	if slots == 0 {
		return 0
	}

	return d.units - d.activeUnits
}

func (d *baseDisk) hasFreeSlots() bool {
	return d.freeSlots() != 0
}

func (d *baseDisk) structValue() ydb_types.Value {
	return ydb_types.StructValue(
		ydb_types.StructFieldValue("id", ydb_types.UTF8Value(d.id)),
		ydb_types.StructFieldValue("image_id", ydb_types.UTF8Value(d.imageID)),
		ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(d.zoneID)),
		ydb_types.StructFieldValue("src_disk_zone_id", ydb_types.UTF8Value(d.srcDiskZoneID)),
		ydb_types.StructFieldValue("src_disk_id", ydb_types.UTF8Value(d.srcDiskID)),
		ydb_types.StructFieldValue("src_disk_checkpoint_id", ydb_types.UTF8Value(d.srcDiskCheckpointID)),
		ydb_types.StructFieldValue("checkpoint_id", ydb_types.UTF8Value(d.checkpointID)),
		ydb_types.StructFieldValue("create_task_id", ydb_types.UTF8Value(d.createTaskID)),
		ydb_types.StructFieldValue("image_size", ydb_types.Uint64Value(d.imageSize)),
		ydb_types.StructFieldValue("size", ydb_types.Uint64Value(d.size)),

		ydb_types.StructFieldValue("active_slots", ydb_types.Uint64Value(d.activeSlots)),
		ydb_types.StructFieldValue("max_active_slots", ydb_types.Uint64Value(d.maxActiveSlots)),
		ydb_types.StructFieldValue("active_units", ydb_types.Uint64Value(d.activeUnits)),
		ydb_types.StructFieldValue("units", ydb_types.Uint64Value(d.units)),
		ydb_types.StructFieldValue("from_pool", ydb_types.BoolValue(d.fromPool)),
		ydb_types.StructFieldValue("retiring", ydb_types.BoolValue(d.retiring)),
		ydb_types.StructFieldValue("deleted_at", persistence.TimestampValue(d.deletedAt)),
		ydb_types.StructFieldValue("status", ydb_types.Int64Value(int64(d.status))),
	)
}

func baseDiskStructTypeString() string {
	return `Struct<
		id: Utf8,
		image_id: Utf8,
		zone_id: Utf8,
		src_disk_zone_id: Utf8,
		src_disk_id: Utf8,
		src_disk_checkpoint_id: Utf8,
		checkpoint_id: Utf8,
		create_task_id: Utf8,
		image_size: Uint64,
		size: Uint64,

		active_slots: Uint64,
		max_active_slots: Uint64,
		active_units: Uint64,
		units: Uint64,
		from_pool: Bool,
		retiring: Bool,
		deleted_at: Timestamp,
		status: Int64>`
}

func baseDisksTableDescription() persistence.CreateTableDescription {
	return persistence.MakeCreateTableDescription(
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("image_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("src_disk_zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("src_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("src_disk_checkpoint_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("checkpoint_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("create_task_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("image_size", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("size", ydb_types.Optional(ydb_types.TypeUint64)),

		persistence.WithColumn("active_slots", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("max_active_slots", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("active_units", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("units", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("from_pool", ydb_types.Optional(ydb_types.TypeBool)),
		persistence.WithColumn("retiring", ydb_types.Optional(ydb_types.TypeBool)),
		persistence.WithColumn("deleted_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("status", ydb_types.Optional(ydb_types.TypeInt64)),

		persistence.WithPrimaryKeyColumn("id"),
	)
}

func scanBaseDisk(res ydb_result.StreamResult) (baseDisk baseDisk, err error) {
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("id", &baseDisk.id),
		ydb_named.OptionalWithDefault("image_id", &baseDisk.imageID),
		ydb_named.OptionalWithDefault("zone_id", &baseDisk.zoneID),
		ydb_named.OptionalWithDefault("src_disk_zone_id", &baseDisk.srcDiskZoneID),
		ydb_named.OptionalWithDefault("src_disk_id", &baseDisk.srcDiskID),
		ydb_named.OptionalWithDefault("src_disk_checkpoint_id", &baseDisk.srcDiskCheckpointID),
		ydb_named.OptionalWithDefault("checkpoint_id", &baseDisk.checkpointID),
		ydb_named.OptionalWithDefault("create_task_id", &baseDisk.createTaskID),
		ydb_named.OptionalWithDefault("image_size", &baseDisk.imageSize),
		ydb_named.OptionalWithDefault("size", &baseDisk.size),

		ydb_named.OptionalWithDefault("active_slots", &baseDisk.activeSlots),
		ydb_named.OptionalWithDefault("max_active_slots", &baseDisk.maxActiveSlots),
		ydb_named.OptionalWithDefault("active_units", &baseDisk.activeUnits),
		ydb_named.OptionalWithDefault("units", &baseDisk.units),
		ydb_named.OptionalWithDefault("from_pool", &baseDisk.fromPool),
		ydb_named.OptionalWithDefault("retiring", &baseDisk.retiring),
		ydb_named.OptionalWithDefault("deleted_at", &baseDisk.deletedAt),
		ydb_named.OptionalWithDefault("status", &baseDisk.status),
	)
	if err != nil {
		return baseDisk, &errors.NonRetriableError{
			Err: fmt.Errorf("scanBaseDisks: failed to parse row: %w", err),
		}
	}
	return baseDisk, res.Err()
}

func scanConfig(res ydb_result.Result) (config poolConfig, err error) {
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("image_id", &config.imageID),
		ydb_named.OptionalWithDefault("zone_id", &config.zoneID),
		ydb_named.OptionalWithDefault("capacity", &config.capacity),
		ydb_named.OptionalWithDefault("image_size", &config.imageSize),
	)
	if err != nil {
		return config, &errors.NonRetriableError{
			Err: fmt.Errorf("scanConfig: failed to parse row: %w", err),
		}
	}
	return config, res.Err()
}

func scanPool(res ydb_result.Result) (pool pool, err error) {
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("image_id", &pool.imageID),
		ydb_named.OptionalWithDefault("zone_id", &pool.zoneID),
		ydb_named.OptionalWithDefault("size", &pool.size),
		ydb_named.OptionalWithDefault("free_units", &pool.freeUnits),
		ydb_named.OptionalWithDefault("acquired_units", &pool.acquiredUnits),
		ydb_named.OptionalWithDefault("base_disks_inflight", &pool.baseDisksInflight),
		ydb_named.OptionalWithDefault("lock_id", &pool.lockID),
		ydb_named.OptionalWithDefault("status", &pool.status),
		ydb_named.OptionalWithDefault("created_at", &pool.createdAt),
	)
	if err != nil {
		return pool, &errors.NonRetriableError{
			Err: fmt.Errorf("getPoolOrDefault: failed to parse row: %w", err),
		}
	}
	if pool.createdAt.IsZero() {
		pool.createdAt = time.Now()
	}
	return pool, res.Err()
}

func scanSlot(slots ydb_result.Result) (slot slot, err error) {
	var diskKind int64
	err = slots.ScanNamed(
		ydb_named.OptionalWithDefault("overlay_disk_id", &slot.overlayDiskID),
		ydb_named.OptionalWithDefault("overlay_disk_kind", &diskKind),
		ydb_named.OptionalWithDefault("overlay_disk_size", &slot.overlayDiskSize),
		ydb_named.OptionalWithDefault("base_disk_id", &slot.baseDiskID),
		ydb_named.OptionalWithDefault("image_id", &slot.imageID),
		ydb_named.OptionalWithDefault("zone_id", &slot.zoneID),
		ydb_named.OptionalWithDefault("allotted_slots", &slot.allottedSlots),
		ydb_named.OptionalWithDefault("allotted_units", &slot.allottedUnits),
		ydb_named.OptionalWithDefault("released_at", &slot.releasedAt),
		ydb_named.OptionalWithDefault("target_base_disk_id", &slot.targetBaseDiskID),
		ydb_named.OptionalWithDefault("target_allotted_slots", &slot.targetAllottedSlots),
		ydb_named.OptionalWithDefault("target_allotted_units", &slot.targetAllottedUnits),
		ydb_named.OptionalWithDefault("generation", &slot.generation),
		ydb_named.OptionalWithDefault("status", &slot.status),
	)
	if err != nil {
		return slot, &errors.NonRetriableError{
			Err: fmt.Errorf("scanSLot: failed to parse row: %w", err),
		}
	}
	slot.overlayDiskKind = types.DiskKind(diskKind)
	return slot, slots.Err()
}

func scanBaseDisks(ctx context.Context, res ydb_result.StreamResult) ([]baseDisk, error) {
	var baseDisks []baseDisk
	for res.NextResultSet(ctx) {
		for res.NextRow() {
			baseDisk, err := scanBaseDisk(res)
			if err != nil {
				return nil, err
			}
			baseDisks = append(baseDisks, baseDisk)
		}
	}
	return baseDisks, res.Err()
}

////////////////////////////////////////////////////////////////////////////////

type baseDiskTransition struct {
	oldState *baseDisk
	state    *baseDisk
}

type slotTransition struct {
	oldState *slot
	state    *slot
}

type poolTransition struct {
	oldState pool
	state    pool
}

type poolAction struct {
	sizeDiff              int64
	freeUnitsDiff         int64
	acquiredUnitsDiff     int64
	baseDisksInflightDiff int64
}

func (a *poolAction) hasChanges() bool {
	return a.sizeDiff != 0 || a.baseDisksInflightDiff != 0
}

func (a *poolAction) apply(state *pool) {
	state.size = uint64(int64(state.size) + a.sizeDiff)
	state.freeUnits = uint64(int64(state.freeUnits) + a.freeUnitsDiff)
	state.acquiredUnits = uint64(int64(state.acquiredUnits) + a.acquiredUnitsDiff)
	state.baseDisksInflight = uint64(
		int64(state.baseDisksInflight) + a.baseDisksInflightDiff,
	)
}

func computePoolAction(t baseDiskTransition) poolAction {
	var a poolAction

	if t.oldState == nil {
		if !t.state.isDoomed() {
			// Add all free slots to pool when creating new disk.
			a.sizeDiff = int64(t.state.freeSlots())
			a.freeUnitsDiff = int64(t.state.freeUnits())
		}

		if t.state.isInflight() {
			a.baseDisksInflightDiff = 1
		}

		return a
	}

	if !t.oldState.fromPool {
		// Returning base disk to pool is forbidden (otherwise it's nothing to
		// do here).
		return a
	}

	switch {
	case !t.state.fromPool || (!t.oldState.isDoomed() && t.state.isDoomed()):
		// Remove all free slots from pool when ejecting disk.
		a.sizeDiff = -int64(t.oldState.freeSlots())
		a.freeUnitsDiff = -int64(t.oldState.freeUnits())
		// TODO: check that t.oldState.activeUnits is 0

	case t.oldState.status == t.state.status && !t.state.isDoomed():
		// Regular transition for healthy base disks.
		a.sizeDiff = int64(t.state.freeSlots()) - int64(t.oldState.freeSlots())
		a.freeUnitsDiff = int64(t.state.freeUnits()) - int64(t.oldState.freeUnits())
		a.acquiredUnitsDiff = int64(t.state.activeUnits) - int64(t.oldState.activeUnits)
	}

	if t.oldState.isInflight() && !t.state.isInflight() {
		a.baseDisksInflightDiff = -1
	}

	// NOTE: already existing base disk can't switch its state from
	// 'not inflight' to 'inflight'.

	return a
}

////////////////////////////////////////////////////////////////////////////////

type slotStatus uint32

func (s *slotStatus) UnmarshalYDB(res ydb_types.RawValue) error {
	*s = slotStatus(res.Int64())
	return nil
}

// NOTE: These values are stored in DB, do not shuffle them around.
const (
	slotStatusAcquired slotStatus = iota
	slotStatusReleased slotStatus = iota
)

func slotStatusToString(status slotStatus) string {
	switch status {
	case slotStatusAcquired:
		return "acquired"
	case slotStatusReleased:
		return "released"
	}

	return fmt.Sprintf("unknown_%v", status)
}

////////////////////////////////////////////////////////////////////////////////

type slot struct {
	overlayDiskID       string
	overlayDiskKind     types.DiskKind
	overlayDiskSize     uint64
	baseDiskID          string
	imageID             string
	zoneID              string
	allottedSlots       uint64
	allottedUnits       uint64
	releasedAt          time.Time
	targetBaseDiskID    string
	targetAllottedSlots uint64
	targetAllottedUnits uint64
	generation          uint64
	status              slotStatus
}

func (s *slot) structValue() ydb_types.Value {
	return ydb_types.StructValue(
		ydb_types.StructFieldValue("overlay_disk_id", ydb_types.UTF8Value(s.overlayDiskID)),
		ydb_types.StructFieldValue("overlay_disk_kind", ydb_types.Int64Value(int64(s.overlayDiskKind))),
		ydb_types.StructFieldValue("overlay_disk_size", ydb_types.Uint64Value(s.overlayDiskSize)),
		ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(s.baseDiskID)),
		ydb_types.StructFieldValue("image_id", ydb_types.UTF8Value(s.imageID)),
		ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(s.zoneID)),
		ydb_types.StructFieldValue("allotted_slots", ydb_types.Uint64Value(s.allottedSlots)),
		ydb_types.StructFieldValue("allotted_units", ydb_types.Uint64Value(s.allottedUnits)),
		ydb_types.StructFieldValue("released_at", persistence.TimestampValue(s.releasedAt)),
		ydb_types.StructFieldValue("target_base_disk_id", ydb_types.UTF8Value(s.targetBaseDiskID)),
		ydb_types.StructFieldValue("target_allotted_slots", ydb_types.Uint64Value(s.targetAllottedSlots)),
		ydb_types.StructFieldValue("target_allotted_units", ydb_types.Uint64Value(s.targetAllottedUnits)),
		ydb_types.StructFieldValue("generation", ydb_types.Uint64Value(s.generation)),
		ydb_types.StructFieldValue("status", ydb_types.Int64Value(int64(s.status))),
	)
}

func slotStructTypeString() string {
	return `Struct<
		overlay_disk_id: Utf8,
		overlay_disk_kind: Int64,
		overlay_disk_size: Uint64,
		base_disk_id: Utf8,
		image_id: Utf8,
		zone_id: Utf8,
		allotted_slots: Uint64,
		allotted_units: Uint64,
		released_at: Timestamp,
		target_base_disk_id: Utf8,
		target_allotted_slots: Uint64,
		target_allotted_units: Uint64,
		generation: Uint64,
		status: Int64>`
}

func slotsTableDescription() persistence.CreateTableDescription {
	return persistence.MakeCreateTableDescription(
		persistence.WithColumn("overlay_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("overlay_disk_kind", ydb_types.Optional(ydb_types.TypeInt64)),
		persistence.WithColumn("overlay_disk_size", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("image_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("status", ydb_types.Optional(ydb_types.TypeInt64)),
		persistence.WithColumn("allotted_slots", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("allotted_units", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("released_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("target_base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("target_allotted_slots", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("target_allotted_units", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("generation", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithPrimaryKeyColumn("overlay_disk_id"),
	)
}

////////////////////////////////////////////////////////////////////////////////

type poolStatus uint32

func (s *poolStatus) UnmarshalYDB(res ydb_types.RawValue) error {
	*s = poolStatus(res.Int64())
	return nil
}

// NOTE: These values are stored in DB, do not shuffle them around.
const (
	poolStatusReady   poolStatus = iota
	poolStatusDeleted poolStatus = iota
)

func poolStatusToString(status poolStatus) string {
	switch status {
	case poolStatusReady:
		return "ready"
	case poolStatusDeleted:
		return "deleted"
	}

	return fmt.Sprintf("unknown_%v", status)
}

type poolConfig struct {
	imageID   string
	zoneID    string
	capacity  uint64
	imageSize uint64
}

type pool struct {
	imageID           string
	zoneID            string
	size              uint64
	freeUnits         uint64
	acquiredUnits     uint64
	baseDisksInflight uint64
	lockID            string
	status            poolStatus
	createdAt         time.Time
}

func (p *pool) structValue() ydb_types.Value {
	return ydb_types.StructValue(
		ydb_types.StructFieldValue("image_id", ydb_types.UTF8Value(p.imageID)),
		ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(p.zoneID)),
		ydb_types.StructFieldValue("size", ydb_types.Uint64Value(p.size)),
		ydb_types.StructFieldValue("free_units", ydb_types.Uint64Value(p.freeUnits)),
		ydb_types.StructFieldValue("acquired_units", ydb_types.Uint64Value(p.acquiredUnits)),
		ydb_types.StructFieldValue("base_disks_inflight", ydb_types.Uint64Value(p.baseDisksInflight)),
		ydb_types.StructFieldValue("lock_id", ydb_types.UTF8Value(p.lockID)),
		ydb_types.StructFieldValue("status", ydb_types.Int64Value(int64(p.status))),
		ydb_types.StructFieldValue("created_at", persistence.TimestampValue(p.createdAt)),
	)
}

func poolStructTypeString() string {
	return `Struct<
		image_id: Utf8,
		zone_id: Utf8,
		size: Uint64,
		acquired_units: Uint64,
		free_units: Uint64,
		base_disks_inflight: Uint64,
		lock_id: Utf8,
		status: Int64,
		created_at: Timestamp>`
}

func poolsTableDescription() persistence.CreateTableDescription {
	return persistence.MakeCreateTableDescription(
		persistence.WithColumn("image_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("size", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("free_units", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("acquired_units", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("base_disks_inflight", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("lock_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("status", ydb_types.Optional(ydb_types.TypeInt64)),
		persistence.WithColumn("created_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithPrimaryKeyColumn("image_id", "zone_id"),
	)
}

////////////////////////////////////////////////////////////////////////////////

// TODO: move some constants to configs.
const (
	// 32 GB.
	baseDiskUnitSize            = uint64(32 << 30)
	minBaseDiskUnits            = 30
	baseDiskOverSubscription    = 2
	overlayDiskOversubscription = 30
	// SSD units should be more valuable than HDD.
	ssdUnitMultiplier = 5
)

func (s *storageYDB) generateBaseDiskForPool(
	imageID string,
	zoneID string,
	imageSize uint64,
	srcDisk *types.Disk,
	srcDiskCheckpointID string,
	srcDiskCheckpointSize uint64,
) baseDisk {

	var requiredSize uint64
	if srcDisk != nil {
		// Base disk will be copied from 'src disk'.
		requiredSize = srcDiskCheckpointSize
	} else {
		requiredSize = imageSize
	}

	var size, maxActiveSlots, units uint64

	if requiredSize == 0 {
		// Default case.
		units = s.maxBaseDiskUnits
		maxActiveSlots = s.maxActiveSlots
	} else {
		// Base disks are using SSD.
		ssdUnits := divideWithRoundingUp(
			requiredSize,
			baseDiskUnitSize,
		)
		size = ssdUnits * baseDiskUnitSize

		units = ssdUnitMultiplier * ssdUnits
		units = baseDiskOverSubscription * units
		units = max(units, minBaseDiskUnits)
		units = min(units, s.maxBaseDiskUnits)

		maxActiveSlots = min(units, s.maxActiveSlots)
	}

	var srcDiskZoneID string
	var srcDiskID string
	if srcDisk != nil {
		srcDiskZoneID = srcDisk.ZoneId
		srcDiskID = srcDisk.DiskId
	}

	return baseDisk{
		id:                  generateDiskID(),
		imageID:             imageID,
		zoneID:              zoneID,
		checkpointID:        imageID, // Note: we use image id as checkpoint id.
		createTaskID:        "",      // Will be determined later.
		imageSize:           imageSize,
		size:                size,
		srcDiskZoneID:       srcDiskZoneID,
		srcDiskID:           srcDiskID,
		srcDiskCheckpointID: srcDiskCheckpointID,

		maxActiveSlots: maxActiveSlots,
		units:          units,
		fromPool:       true,
		status:         baseDiskStatusScheduling,
	}
}

func computeAllottedUnits(s *slot) uint64 {
	var res uint64

	overlayDiskUnits := s.overlayDiskSize / baseDiskUnitSize

	switch s.overlayDiskKind {
	case types.DiskKind_DISK_KIND_SSD:
		res = divideWithRoundingUp(
			ssdUnitMultiplier*overlayDiskUnits,
			overlayDiskOversubscription,
		)
	case types.DiskKind_DISK_KIND_HDD:
		res = divideWithRoundingUp(
			overlayDiskUnits,
			overlayDiskOversubscription,
		)
	}

	// Can't allocate less than 1 unit.
	return max(1, res)
}

////////////////////////////////////////////////////////////////////////////////

func acquireUnitsAndSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	disk *baseDisk,
	slot *slot,
) error {

	if !disk.hasFreeSlots() {
		err := tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"acquireUnitsAndSlots failed: base disk has no free slots, disk=%v, slot=%v",
				disk,
				slot,
			),
		}
	}

	slot.allottedSlots = 1
	disk.activeSlots++

	slot.allottedUnits = computeAllottedUnits(slot)
	disk.activeUnits += slot.allottedUnits

	return nil
}

func acquireTargetUnitsAndSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	disk *baseDisk,
	slot *slot,
) error {

	if !disk.hasFreeSlots() {
		err := tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"acquireTargetUnitsAndSlots failed: base disk has no free slots, disk=%v, slot=%v",
				disk,
				slot,
			),
		}
	}

	slot.targetAllottedSlots = 1
	disk.activeSlots++

	slot.targetAllottedUnits = computeAllottedUnits(slot)
	disk.activeUnits += slot.targetAllottedUnits

	return nil
}

func releaseUnitsAndSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	disk *baseDisk,
	slot slot,
) error {

	if slot.allottedSlots == 0 {
		if disk.activeSlots == 0 {
			err := tx.Commit(ctx)
			if err != nil {
				return err
			}

			return &errors.NonRetriableError{
				Err: fmt.Errorf(
					"internal inconsistency: disk=%v activeSlots should be greater than zero",
					disk,
				),
			}
		}

		disk.activeSlots--
	} else {
		if disk.activeSlots < slot.allottedSlots {
			err := tx.Commit(ctx)
			if err != nil {
				return err
			}

			return &errors.NonRetriableError{
				Err: fmt.Errorf(
					"internal inconsistency: disk=%v activeSlots should be greater than allottedSlots=%v",
					disk,
					slot.allottedSlots,
				),
			}
		}

		disk.activeSlots -= slot.allottedSlots
	}

	if disk.activeUnits < slot.allottedUnits {
		err := tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"internal inconsistency: disk=%v activeUnits should be greater than allottedUnits=%v",
				disk,
				slot.allottedUnits,
			),
		}
	}
	disk.activeUnits -= slot.allottedUnits

	return nil
}

func releaseTargetUnitsAndSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	disk *baseDisk,
	slot slot,
) error {

	if slot.targetAllottedSlots == 0 {
		if disk.activeSlots == 0 {
			err := tx.Commit(ctx)
			if err != nil {
				return err
			}

			return &errors.NonRetriableError{
				Err: fmt.Errorf(
					"internal inconsistency: disk=%v activeSlots should be greater than zero",
					disk,
				),
			}
		}

		disk.activeSlots--
	} else {
		if disk.activeSlots < slot.targetAllottedSlots {
			err := tx.Commit(ctx)
			if err != nil {
				return err
			}

			return &errors.NonRetriableError{
				Err: fmt.Errorf(
					"internal inconsistency: disk=%v activeSlots should be greater than targetAllottedSlots=%v",
					disk,
					slot.targetAllottedSlots,
				),
			}
		}

		disk.activeSlots -= slot.targetAllottedSlots
	}

	if disk.activeUnits < slot.targetAllottedUnits {
		err := tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"internal inconsistency: disk=%v activeUnits should be greater than targetAllottedUnits=%v",
				disk,
				slot.targetAllottedUnits,
			),
		}
	}
	disk.activeUnits -= slot.targetAllottedUnits

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type baseDiskKey struct {
	imageID    string
	zoneID     string
	baseDiskID string
}

func (k *baseDiskKey) structValue() ydb_types.Value {
	return ydb_types.StructValue(
		ydb_types.StructFieldValue("image_id", ydb_types.UTF8Value(k.imageID)),
		ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(k.zoneID)),
		ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(k.baseDiskID)),
	)
}

func baseDiskKeyStructTypeString() string {
	return `Struct<
		image_id: Utf8,
		zone_id: Utf8,
		base_disk_id: Utf8>`
}

func keyFromBaseDisk(baseDisk *baseDisk) baseDiskKey {
	return baseDiskKey{
		imageID:    baseDisk.imageID,
		zoneID:     baseDisk.zoneID,
		baseDiskID: baseDisk.id,
	}
}

////////////////////////////////////////////////////////////////////////////////

func CreateYDBTables(
	ctx context.Context,
	config *pools_config.PoolsConfig,
	db *persistence.YDBClient,
) error {

	logging.Info(ctx, "Creating tables for pools in %v", db.AbsolutePath(config.GetStorageFolder()))

	err := db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "base_disks", baseDisksTableDescription())
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created base_disks table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "slots", slotsTableDescription())
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created slots table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "overlay_disk_ids", persistence.MakeCreateTableDescription(
		persistence.WithColumn("base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("overlay_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("base_disk_id", "overlay_disk_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created overlay_disk_ids table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "configs", persistence.MakeCreateTableDescription(
		persistence.WithColumn("image_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("kind", ydb_types.Optional(ydb_types.TypeInt64)),
		persistence.WithColumn("capacity", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("image_size", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithPrimaryKeyColumn("image_id", "zone_id", "kind"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created configs table")

	// Contains base disk ids that currently have status "scheduling".
	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "scheduling", persistence.MakeCreateTableDescription(
		persistence.WithColumn("image_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("image_id", "zone_id", "base_disk_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created scheduling table")

	// Contains base disk ids that currently have free (not acquired) slots.
	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "free", persistence.MakeCreateTableDescription(
		persistence.WithColumn("image_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("image_id", "zone_id", "base_disk_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created free table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "pools", poolsTableDescription())
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created pools table")

	// Contains base disk ids that currently have status "deleting".
	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "deleting", persistence.MakeCreateTableDescription(
		persistence.WithColumn("base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("base_disk_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created deleting table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "deleted", persistence.MakeCreateTableDescription(
		persistence.WithColumn("deleted_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("base_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("deleted_at", "base_disk_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created deleted table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "released", persistence.MakeCreateTableDescription(
		persistence.WithColumn("released_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("overlay_disk_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("released_at", "overlay_disk_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created released table")

	logging.Info(ctx, "Created tables for pools")

	return nil
}

func DropYDBTables(
	ctx context.Context,
	config *pools_config.PoolsConfig,
	db *persistence.YDBClient,
) error {

	logging.Info(ctx, "Dropping tables for pools in %v", db.AbsolutePath(config.GetStorageFolder()))

	err := db.DropTable(ctx, config.GetStorageFolder(), "base_disks")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped base_disks table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "slots")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped slots table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "overlay_disk_ids")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped overlay_disk_ids table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "configs")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped configs table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "scheduling")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped scheduling table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "free")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped free table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "pools")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped pools table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "deleting")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped deleting table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "deleted")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped deleted table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "released")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped released table")

	logging.Info(ctx, "Dropped tables for pools")

	return nil
}
