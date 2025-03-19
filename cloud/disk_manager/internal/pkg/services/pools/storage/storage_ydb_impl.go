package storage

import (
	"context"
	"fmt"
	"time"

	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_result "github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) findBaseDisks(
	ctx context.Context,
	session *persistence.Session,
	ids []string,
) ([]baseDisk, error) {

	if len(ids) == 0 {
		return nil, nil
	}

	values := make([]ydb_types.Value, 0)
	for _, id := range ids {
		values = append(values, ydb_types.UTF8Value(id))
	}

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $ids as List<Utf8>;

		select *
		from base_disks
		where id in $ids
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$ids", ydb_types.ListValue(values...)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	baseDisks, err := scanBaseDisks(ctx, res)
	if err != nil {
		return nil, err
	}

	return baseDisks, nil
}

func (s *storageYDB) findBaseDisksTx(
	ctx context.Context,
	tx *persistence.Transaction,
	ids []string,
) ([]baseDisk, error) {

	if len(ids) == 0 {
		return nil, nil
	}

	values := make([]ydb_types.Value, 0)
	for _, id := range ids {
		values = append(values, ydb_types.UTF8Value(id))
	}

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $ids as List<Utf8>;

		select *
		from base_disks
		where id in $ids
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$ids", ydb_types.ListValue(values...)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	baseDisks, err := scanBaseDisks(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return nil, commitErr
		}

		return nil, err
	}

	return baseDisks, nil
}

func (s *storageYDB) findBaseDisk(
	ctx context.Context,
	tx *persistence.Transaction,
	id string,
) (*baseDisk, error) {

	baseDisks, err := s.findBaseDisksTx(ctx, tx, []string{id})
	if err != nil {
		return nil, err
	}

	if len(baseDisks) == 0 {
		return nil, nil
	}

	return &baseDisks[0], nil
}

func (s *storageYDB) getBaseDisk(
	ctx context.Context,
	tx *persistence.Transaction,
	id string,
) (baseDisk, error) {

	out, err := s.findBaseDisk(ctx, tx, id)
	if err != nil {
		return baseDisk{}, err
	}

	if out == nil {
		err = tx.Commit(ctx)
		if err != nil {
			return baseDisk{}, err
		}

		return baseDisk{}, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"failed to find base disk with id=%v",
				id,
			),
		}
	}

	return *out, nil
}

func (s *storageYDB) acquireBaseDiskSlotIdempotent(
	ctx context.Context,
	tx *persistence.Transaction,
	slot slot,
) (BaseDisk, error) {

	if slot.status == slotStatusReleased {
		err := tx.Commit(ctx)
		if err != nil {
			return BaseDisk{}, err
		}

		return BaseDisk{}, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"slot=%v is already released",
				slot,
			),
			Silent: true,
		}
	}

	baseDisk, err := s.getBaseDisk(ctx, tx, slot.baseDiskID)
	if err != nil {
		return BaseDisk{}, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return BaseDisk{}, err
	}

	if baseDisk.isDoomed() {
		return BaseDisk{}, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"base disk with id=%v is already deleting/deleted, slot=%v",
				baseDisk.id,
				slot,
			),
		}
	}

	return baseDisk.toBaseDisk(), nil
}

func (s *storageYDB) upsertIntoFreeTable(
	ctx context.Context,
	tx *persistence.Transaction,
	baseDisk *baseDisk,
) error {

	key := keyFromBaseDisk(baseDisk)

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $keys as List<%v>;

		upsert into free
		select *
		from AS_TABLE($keys)
	`, s.tablesPath, baseDiskKeyStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$keys", ydb_types.ListValue(key.structValue())),
	))
	return err
}

func (s *storageYDB) updateFreeTable(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []baseDiskTransition,
) error {

	toDelete := make([]ydb_types.Value, 0)
	toUpsert := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		key := keyFromBaseDisk(t.state)

		oldFree := t.oldState != nil && t.oldState.hasFreeSlots()
		free := t.state.hasFreeSlots()

		switch {
		case oldFree && !free:
			toDelete = append(toDelete, key.structValue())
		case !oldFree && free:
			toUpsert = append(toUpsert, key.structValue())
		}
	}

	if len(toDelete) != 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $values as List<%v>;

			delete from free on
			select *
			from AS_TABLE($values)
		`, s.tablesPath, baseDiskKeyStructTypeString()), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$values", ydb_types.ListValue(toDelete...)),
		))
		if err != nil {
			return err
		}
	}

	if len(toUpsert) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<%v>;

		upsert into free
		select *
		from AS_TABLE($values)
	`, s.tablesPath, baseDiskKeyStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(toUpsert...)),
	))
	return err
}

func (s *storageYDB) updateSchedulingTable(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []baseDiskTransition,
) error {

	keys := make([]ydb_types.Value, 0)
	for _, t := range transitions {
		if t.oldState != nil &&
			t.oldState.status != t.state.status &&
			t.oldState.status == baseDiskStatusScheduling {

			key := keyFromBaseDisk(t.state)
			keys = append(keys, key.structValue())
		}
	}

	if len(keys) != 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $keys as List<%v>;

			delete from scheduling on
			select *
			from AS_TABLE($keys)
		`, s.tablesPath, baseDiskKeyStructTypeString()), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$keys", ydb_types.ListValue(keys...)),
		))
		if err != nil {
			return err
		}
	}

	keys = make([]ydb_types.Value, 0)
	for _, t := range transitions {
		if (t.oldState == nil || t.oldState.status != t.state.status) &&
			t.state.status == baseDiskStatusScheduling {

			key := keyFromBaseDisk(t.state)
			keys = append(keys, key.structValue())
		}
	}

	if len(keys) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $keys as List<%v>;

		upsert into scheduling
		select *
		from AS_TABLE($keys)
	`, s.tablesPath, baseDiskKeyStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$keys", ydb_types.ListValue(keys...)),
	))
	return err
}

func (s *storageYDB) updateDeletingTable(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []baseDiskTransition,
) error {

	values := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.oldState != nil &&
			t.oldState.status != t.state.status &&
			t.oldState.status == baseDiskStatusDeleting {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(t.state.id)),
			))
		}
	}

	if len(values) != 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $values as List<Struct<base_disk_id: Utf8>>;

			delete from deleting on
			select *
			from AS_TABLE($values)
		`, s.tablesPath), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
		))
		if err != nil {
			return err
		}
	}

	values = make([]ydb_types.Value, 0)
	for _, t := range transitions {
		if (t.oldState == nil || t.oldState.status != t.state.status) &&
			t.state.status == baseDiskStatusDeleting {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(t.state.id)),
			))
		}
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<Struct<base_disk_id: Utf8>>;

		upsert into deleting
		select *
		from AS_TABLE($values)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) upsertSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	slots []slot,
) error {

	values := make([]ydb_types.Value, 0)
	for _, slot := range slots {
		values = append(values, slot.structValue())
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<%v>;

		upsert into slots
		select *
		from AS_TABLE($values)
	`, s.tablesPath, slotStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) upsertSlot(
	ctx context.Context,
	tx *persistence.Transaction,
	st slot,
) error {

	return s.upsertSlots(ctx, tx, []slot{st})
}

func (s *storageYDB) updateOverlayDiskIDs(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []slotTransition,
) error {

	values := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.oldState == nil {
			continue
		}

		if (t.oldState.status != t.state.status && t.state.status == slotStatusReleased) ||
			(t.oldState.baseDiskID != t.state.baseDiskID) {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(t.oldState.baseDiskID)),
				ydb_types.StructFieldValue("overlay_disk_id", ydb_types.UTF8Value(t.state.overlayDiskID)),
			))
		}
	}

	if len(values) != 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $values as List<Struct<base_disk_id: Utf8, overlay_disk_id: Utf8>>;

			delete from overlay_disk_ids on
			select *
			from AS_TABLE($values)
		`, s.tablesPath), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
		))
		if err != nil {
			return err
		}
	}

	values = make([]ydb_types.Value, 0)
	for _, t := range transitions {
		if t.state.status == slotStatusAcquired &&
			(t.oldState == nil || t.oldState.baseDiskID != t.state.baseDiskID) {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(t.state.baseDiskID)),
				ydb_types.StructFieldValue("overlay_disk_id", ydb_types.UTF8Value(t.state.overlayDiskID)),
			))
		}
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<Struct<base_disk_id: Utf8, overlay_disk_id: Utf8>>;

		upsert into overlay_disk_ids
		select *
		from AS_TABLE($values)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) updateSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []slotTransition,
) error {

	err := s.updateOverlayDiskIDs(ctx, tx, transitions)
	if err != nil {
		return err
	}

	values := make([]ydb_types.Value, 0)

	// Deletion from 'released' table is not permitted via slot transition.

	for _, t := range transitions {
		if (t.oldState == nil || t.oldState.status != t.state.status) &&
			t.state.status == slotStatusReleased {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("released_at", persistence.TimestampValue(t.state.releasedAt)),
				ydb_types.StructFieldValue("overlay_disk_id", ydb_types.UTF8Value(t.state.overlayDiskID)),
			))
		}
	}

	if len(values) != 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $values as List<Struct<released_at: Timestamp, overlay_disk_id: Utf8>>;

			upsert into released
			select *
			from AS_TABLE($values)
		`, s.tablesPath), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
		))
		if err != nil {
			return err
		}
	}

	values = make([]ydb_types.Value, 0)
	for _, t := range transitions {
		if t.oldState == nil || *t.oldState != *t.state {
			values = append(values, t.state.structValue())
		}
	}

	if len(values) == 0 {
		return nil
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<%v>;

		upsert into slots
		select *
		from AS_TABLE($values)
	`, s.tablesPath, slotStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) updateSlot(
	ctx context.Context,
	tx *persistence.Transaction,
	transition slotTransition,
) error {

	return s.updateSlots(ctx, tx, []slotTransition{transition})
}

func (s *storageYDB) upsertIntoBaseDisksTable(
	ctx context.Context,
	tx *persistence.Transaction,
	baseDisk *baseDisk,
) error {

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $base_disks as List<%v>;

		upsert into base_disks
		select *
		from AS_TABLE($base_disks)
	`, s.tablesPath, baseDiskStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$base_disks", ydb_types.ListValue(baseDisk.structValue())),
	))
	return err
}

func (s *storageYDB) updateDeletedTable(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []baseDiskTransition,
) error {

	// Deletion from 'deleted' table is not permitted via base disk transition.

	values := make([]ydb_types.Value, 0)
	for _, t := range transitions {
		if (t.oldState == nil || t.oldState.status != t.state.status) &&
			t.state.status == baseDiskStatusDeleted {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("deleted_at", persistence.TimestampValue(t.state.deletedAt)),
				ydb_types.StructFieldValue("base_disk_id", ydb_types.UTF8Value(t.state.id)),
			))
		}
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<Struct<deleted_at: Timestamp, base_disk_id: Utf8>>;

		upsert into deleted
		select *
		from AS_TABLE($values)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) makeSelectPoolsQuery() string {
	return fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select *
		from pools
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath)
}

func (s *storageYDB) getPoolOrDefault(
	ctx context.Context,
	tx *persistence.Transaction,
	imageID string,
	zoneID string,
) (pool, error) {

	res, err := tx.Execute(
		ctx,
		s.makeSelectPoolsQuery(),
		ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
			ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
		),
	)
	if err != nil {
		return pool{}, err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return pool{
			imageID:   imageID,
			zoneID:    zoneID,
			status:    poolStatusReady,
			createdAt: time.Now(),
		}, res.Err()
	}
	return scanPool(res)
}

func (s *storageYDB) applyBaseDiskInvariants(
	ctx context.Context,
	tx *persistence.Transaction,
	baseDiskTransitions []baseDiskTransition,
) ([]poolTransition, error) {

	poolTransitions := make(map[string]poolTransition)

	for _, baseDiskTransition := range baseDiskTransitions {
		baseDisk := baseDiskTransition.state
		action := computePoolAction(baseDiskTransition)

		if action.hasChanges() {
			imageID := baseDisk.imageID
			zoneID := baseDisk.zoneID
			key := imageID + zoneID

			t, ok := poolTransitions[key]
			if !ok {
				p, err := s.getPoolOrDefault(ctx, tx, imageID, zoneID)
				if err != nil {
					return nil, err
				}

				t = poolTransition{
					oldState: p,
					state:    p,
				}
			}

			if t.state.status == poolStatusDeleted {
				// Remove from deleted pool.
				baseDisk.fromPool = false
			} else {
				action.apply(&t.state)
			}

			poolTransitions[key] = t
		}

		baseDisk.applyInvariants()
	}

	values := make([]poolTransition, 0)
	for _, v := range poolTransitions {
		values = append(values, v)
	}

	return values, nil
}

func (s *storageYDB) updatePoolsTable(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []poolTransition,
) error {

	values := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.oldState != t.state {
			values = append(values, t.state.structValue())
		}
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $pools as List<%v>;

		upsert into pools
		select *
		from AS_TABLE($pools)
	`, s.tablesPath, poolStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$pools", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) updateBaseDisks(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []baseDiskTransition,
) error {

	poolTransitions, err := s.applyBaseDiskInvariants(ctx, tx, transitions)
	if err != nil {
		return err
	}

	err = s.updateSchedulingTable(ctx, tx, transitions)
	if err != nil {
		return err
	}

	err = s.updateDeletingTable(ctx, tx, transitions)
	if err != nil {
		return err
	}

	err = s.updateDeletedTable(ctx, tx, transitions)
	if err != nil {
		return err
	}

	err = s.updateFreeTable(ctx, tx, transitions)
	if err != nil {
		return err
	}

	err = s.updatePoolsTable(ctx, tx, poolTransitions)
	if err != nil {
		return err
	}

	values := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.oldState == nil || *t.oldState != *t.state {
			values = append(values, t.state.structValue())
		}
	}

	if len(values) == 0 {
		return nil
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $base_disks as List<%v>;

		upsert into base_disks
		select *
		from AS_TABLE($base_disks)
	`, s.tablesPath, baseDiskStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$base_disks", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) updateBaseDisk(
	ctx context.Context,
	tx *persistence.Transaction,
	transition baseDiskTransition,
) error {

	return s.updateBaseDisks(ctx, tx, []baseDiskTransition{transition})
}

func (s *storageYDB) updateBaseDisksAndSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	baseDiskTransitions []baseDiskTransition,
	slotTransitions []slotTransition,
) error {

	err := s.updateBaseDisks(ctx, tx, baseDiskTransitions)
	if err != nil {
		return err
	}

	return s.updateSlots(ctx, tx, slotTransitions)
}

func (s *storageYDB) updateBaseDiskAndSlot(
	ctx context.Context,
	tx *persistence.Transaction,
	bt baseDiskTransition,
	st slotTransition,
) error {

	return s.updateBaseDisksAndSlots(
		ctx,
		tx,
		[]baseDiskTransition{bt},
		[]slotTransition{st},
	)
}

func (s *storageYDB) checkPoolConfigured(
	ctx context.Context,
	tx *persistence.Transaction,
	imageID string,
	zoneID string,
) (capacity uint64, err error) {

	read, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select capacity
		from configs
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return
	}
	defer read.Close()

	var rows int
	for read.NextResultSet(ctx) {
		for read.NextRow() {
			rows++
			var c uint64
			err = read.ScanNamed(
				ydb_named.OptionalWithDefault("capacity", &c),
			)
			if err != nil {
				return 0, &errors.NonRetriableError{
					Err: fmt.Errorf("checkPoolConfigured: failed to parse row: %w", err),
				}
			}

			capacity += c
		}
	}

	if rows == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return
		}

		err = &errors.NonRetriableError{
			Err: fmt.Errorf(
				"pool should be configured, imageID=%v, zoneID=%v",
				imageID,
				zoneID,
			),
		}
		return
	}

	return
}

// Just an overloading for |isPoolConfigured|.
func (s *storageYDB) isPoolConfiguredTx(
	ctx context.Context,
	tx *persistence.Transaction,
	imageID string,
	zoneID string,
) (bool, error) {

	read, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select count(*)
		from configs
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return false, err
	}
	defer read.Close()

	if !read.NextResultSet(ctx) || !read.NextRow() {
		return false, read.Err()
	}

	var count uint64
	err = read.ScanWithDefaults(&count)
	if err != nil {
		return false, &errors.NonRetriableError{
			Err: fmt.Errorf("isPoolConfiguredTx: failed to parse row: %w", err),
		}
	}

	return count != 0, nil
}

func (s *storageYDB) isPoolConfigured(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	zoneID string,
) (bool, error) {

	_, read, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select count(*)
		from configs
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return false, err
	}
	defer read.Close()

	if !read.NextResultSet(ctx) || !read.NextRow() {
		return false, read.Err()
	}

	var count uint64
	err = read.ScanWithDefaults(&count)
	if err != nil {
		return false, &errors.NonRetriableError{
			Err: fmt.Errorf("isPoolConfigured: failed to parse row: %w", err),
		}
	}

	return count != 0, nil
}

func (s *storageYDB) acquireBaseDiskSlot(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	st Slot,
) (BaseDisk, error) {

	zoneID := st.OverlayDisk.ZoneId
	overlayDiskID := st.OverlayDisk.DiskId

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return BaseDisk{}, err
	}
	defer tx.Rollback(ctx)

	slots, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $overlay_disk_id as Utf8;

		select *
		from slots
		where overlay_disk_id = $overlay_disk_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$overlay_disk_id", ydb_types.UTF8Value(overlayDiskID)),
	))
	if err != nil {
		return BaseDisk{}, err
	}
	defer slots.Close()

	if slots.NextResultSet(ctx) && slots.NextRow() {
		scannedSlot, err := scanSlot(slots)
		if err != nil {
			commitErr := tx.Commit(ctx)
			if commitErr != nil {
				return BaseDisk{}, commitErr
			}

			return BaseDisk{}, err
		}

		return s.acquireBaseDiskSlotIdempotent(ctx, tx, scannedSlot)
	}

	free, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select *
		from free
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return BaseDisk{}, err
	}
	defer free.Close()

	capacity, err := s.checkPoolConfigured(ctx, tx, imageID, zoneID)
	if err != nil {
		return BaseDisk{}, err
	}

	for free.NextResultSet(ctx) {
		for free.NextRow() {
			var baseDiskID string
			err = free.ScanNamed(
				ydb_named.OptionalWithDefault("base_disk_id", &baseDiskID),
			)
			if err != nil {
				return BaseDisk{}, &errors.NonRetriableError{
					Err: fmt.Errorf("acquireBaseDiskSlot: failed to parse row: %w", err),
				}
			}

			baseDisk, err := s.getBaseDisk(ctx, tx, baseDiskID)
			if err != nil {
				return BaseDisk{}, err
			}

			if baseDisk.status != baseDiskStatusCreating &&
				baseDisk.status != baseDiskStatusReady {
				// Disk is not suitable for acquiring.
				continue
			}

			slot := &slot{
				overlayDiskID:   overlayDiskID,
				overlayDiskKind: st.OverlayDiskKind,
				overlayDiskSize: st.OverlayDiskSize,
				baseDiskID:      baseDisk.id,
				imageID:         baseDisk.imageID,
				zoneID:          st.OverlayDisk.ZoneId,
				status:          slotStatusAcquired,
			}

			baseDiskOldState := baseDisk
			err = acquireUnitsAndSlots(ctx, tx, &baseDisk, slot)
			if err != nil {
				return BaseDisk{}, err
			}

			err = s.updateBaseDiskAndSlot(
				ctx,
				tx,
				baseDiskTransition{
					oldState: &baseDiskOldState,
					state:    &baseDisk,
				},
				slotTransition{
					oldState: nil,
					state:    slot,
				},
			)
			if err != nil {
				return BaseDisk{}, err
			}

			err = tx.Commit(ctx)
			if err != nil {
				return BaseDisk{}, err
			}

			logging.Debug(
				ctx,
				"AcquireBaseDiskSlot: acquired slot=%v on baseDisk=%v",
				slot,
				baseDisk,
			)
			return baseDisk.toBaseDisk(), nil
		}
	}

	if capacity == 0 {
		// Zero capacity means that this pool should be created "on demand".
		_, err = tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $image_id as Utf8;
			declare $zone_id as Utf8;
			declare $kind as Int64;
			declare $capacity as Uint64;

			upsert into configs (image_id, zone_id, kind, capacity)
			values ($image_id, $zone_id, $kind, $capacity)
		`, s.tablesPath), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
			ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
			ydb_table.ValueParam("$kind", ydb_types.Int64Value(0)), // TODO: get rid of this param.
			ydb_table.ValueParam("$capacity", ydb_types.Uint64Value(1)),
		))
		if err != nil {
			return BaseDisk{}, err
		}
	}

	err = tx.Commit(ctx)
	if err != nil {
		return BaseDisk{}, err
	}

	return BaseDisk{}, &errors.InterruptExecutionError{}
}

func (s *storageYDB) releaseBaseDiskSlot(
	ctx context.Context,
	session *persistence.Session,
	overlayDisk *types.Disk,
) (BaseDisk, error) {

	overlayDiskID := overlayDisk.DiskId

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return BaseDisk{}, err
	}
	defer tx.Rollback(ctx)

	slots, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $overlay_disk_id as Utf8;

		select *
		from slots
		where overlay_disk_id = $overlay_disk_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$overlay_disk_id", ydb_types.UTF8Value(overlayDiskID)),
	))
	if err != nil {
		return BaseDisk{}, err
	}
	defer slots.Close()

	if !slots.NextResultSet(ctx) || !slots.NextRow() {
		// Should be idempotent.
		return BaseDisk{}, tx.Commit(ctx)
	}

	slot, err := scanSlot(slots)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return BaseDisk{}, commitErr
		}

		return BaseDisk{}, err
	}

	found, err := s.findBaseDisk(ctx, tx, slot.baseDiskID)
	if err != nil {
		return BaseDisk{}, err
	}

	if found == nil {
		// Should be idempotent.
		return BaseDisk{}, tx.Commit(ctx)
	}

	baseDisk := *found

	if slot.status == slotStatusReleased {
		err = tx.Commit(ctx)
		if err != nil {
			return BaseDisk{}, err
		}

		return baseDisk.toBaseDisk(), nil
	}

	slotOldState := slot
	slot.releasedAt = time.Now()
	slot.status = slotStatusReleased

	slotTransitions := make([]slotTransition, 0)
	slotTransitions = append(slotTransitions, slotTransition{
		oldState: &slotOldState,
		state:    &slot,
	})

	baseDiskOldState := baseDisk
	err = releaseUnitsAndSlots(ctx, tx, &baseDisk, slot)
	if err != nil {
		return BaseDisk{}, err
	}

	baseDiskTransitions := make([]baseDiskTransition, 0)
	baseDiskTransitions = append(baseDiskTransitions, baseDiskTransition{
		oldState: &baseDiskOldState,
		state:    &baseDisk,
	})

	if len(slot.targetBaseDiskID) != 0 {
		baseDisk, err := s.getBaseDisk(ctx, tx, slot.targetBaseDiskID)
		if err != nil {
			return BaseDisk{}, err
		}

		baseDiskOldState := baseDisk
		err = releaseTargetUnitsAndSlots(ctx, tx, &baseDisk, slot)
		if err != nil {
			return BaseDisk{}, err
		}

		baseDiskTransitions = append(baseDiskTransitions, baseDiskTransition{
			oldState: &baseDiskOldState,
			state:    &baseDisk,
		})
	}

	err = s.updateBaseDisksAndSlots(ctx, tx, baseDiskTransitions, slotTransitions)
	if err != nil {
		return BaseDisk{}, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return BaseDisk{}, err
	}

	logging.Debug(
		ctx,
		"ReleaseBaseDiskSlot: released slot=%v on baseDisk=%v",
		slot,
		baseDisk,
	)
	return baseDisk.toBaseDisk(), nil
}

func (s *storageYDB) overlayDiskRebasing(
	ctx context.Context,
	session *persistence.Session,
	info RebaseInfo,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	slots, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $overlay_disk_id as Utf8;

		select *
		from slots
		where overlay_disk_id = $overlay_disk_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam(
			"$overlay_disk_id",
			ydb_types.UTF8Value(info.OverlayDisk.DiskId),
		),
	))
	if err != nil {
		return err
	}
	defer slots.Close()

	if !slots.NextResultSet(ctx) || !slots.NextRow() {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"failed to find slot with id=%v",
				info.OverlayDisk.DiskId,
			),
			Silent: true,
		}
	}

	slot, err := scanSlot(slots)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if slot.status == slotStatusReleased {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"slot with id=%v is already released",
				info.OverlayDisk.DiskId,
			),
			Silent: true,
		}
	}

	if slot.generation != info.SlotGeneration {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"using wrong generation, expected=%v, actual=%v",
				info.SlotGeneration,
				slot.generation,
			),
		}
	}

	if len(slot.targetBaseDiskID) != 0 && slot.targetBaseDiskID != info.TargetBaseDiskID {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf("another rebase is in progress for slot=%v", slot),
		}
	}

	found, err := s.findBaseDisk(ctx, tx, info.TargetBaseDiskID)
	if err != nil {
		return err
	}

	if found == nil || found.isDoomed() {
		slotOldState := slot

		// Abort rebasing.
		slot.targetBaseDiskID = ""
		slot.targetAllottedSlots = 0
		slot.targetAllottedUnits = 0

		err = s.updateSlot(
			ctx,
			tx,
			slotTransition{
				oldState: &slotOldState,
				state:    &slot,
			},
		)
		if err != nil {
			return err
		}

		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"rebase is aborted, target base disk with id=%v is already deleting/deleted",
				info.TargetBaseDiskID,
			),
			Silent: true,
		}
	}

	baseDisk := *found

	if slot.baseDiskID == info.TargetBaseDiskID {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	if slot.targetBaseDiskID == info.TargetBaseDiskID {
		// Target units are already acquired.

		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		if baseDisk.status != baseDiskStatusReady {
			// Should wait until target base disk is ready.
			return &errors.InterruptExecutionError{}
		}

		return nil
	}

	baseDiskOldState := baseDisk
	slotOldState := slot

	err = acquireTargetUnitsAndSlots(ctx, tx, &baseDisk, &slot)
	if err != nil {
		return err
	}

	slot.targetBaseDiskID = info.TargetBaseDiskID

	err = s.updateBaseDiskAndSlot(
		ctx,
		tx,
		baseDiskTransition{
			oldState: &baseDiskOldState,
			state:    &baseDisk,
		},
		slotTransition{
			oldState: &slotOldState,
			state:    &slot,
		},
	)
	if err != nil {
		return err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return err
	}

	logging.Info(ctx, "rebasing slot %v to %v", slot, baseDisk)

	if baseDisk.status != baseDiskStatusReady {
		// Should wait until target base disk is ready.
		return &errors.InterruptExecutionError{}
	}

	return nil
}

func (s *storageYDB) overlayDiskRebased(
	ctx context.Context,
	session *persistence.Session,
	info RebaseInfo,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	slots, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $overlay_disk_id as Utf8;

		select *
		from slots
		where overlay_disk_id = $overlay_disk_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam(
			"$overlay_disk_id",
			ydb_types.UTF8Value(info.OverlayDisk.DiskId),
		),
	))
	if err != nil {
		return err
	}
	defer slots.Close()

	if !slots.NextResultSet(ctx) || !slots.NextRow() {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"failed to find slot with id=%v",
				info.OverlayDisk.DiskId,
			),
			Silent: true,
		}
	}

	slot, err := scanSlot(slots)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if slot.status == slotStatusReleased {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"slot with id=%v is already released",
				info.OverlayDisk.DiskId,
			),
			Silent: true,
		}
	}

	if slot.generation != info.SlotGeneration {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"using wrong generation, expected=%v, actual=%v",
				info.SlotGeneration,
				slot.generation,
			),
		}
	}

	if slot.baseDiskID == info.TargetBaseDiskID {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	if slot.targetBaseDiskID != info.TargetBaseDiskID {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf("another rebase is in progress for slot=%v", slot),
		}
	}

	baseDisk, err := s.getBaseDisk(ctx, tx, slot.baseDiskID)
	if err != nil {
		return err
	}

	baseDiskOldState := baseDisk
	err = releaseUnitsAndSlots(ctx, tx, &baseDisk, slot)
	if err != nil {
		return err
	}

	slotOldState := slot

	slot.baseDiskID = slot.targetBaseDiskID
	slot.allottedSlots = slot.targetAllottedSlots
	slot.allottedUnits = slot.targetAllottedUnits

	// Finish rebasing.
	slot.targetBaseDiskID = ""
	slot.targetAllottedSlots = 0
	slot.targetAllottedUnits = 0

	err = s.updateBaseDiskAndSlot(
		ctx,
		tx,
		baseDiskTransition{
			oldState: &baseDiskOldState,
			state:    &baseDisk,
		},
		slotTransition{
			oldState: &slotOldState,
			state:    &slot,
		},
	)
	if err != nil {
		return err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return err
	}

	logging.Info(ctx, "rebased slot %v from %v", slot, baseDisk)
	return nil
}

func (s *storageYDB) baseDiskCreated(
	ctx context.Context,
	session *persistence.Session,
	baseDisk BaseDisk,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	found, err := s.getBaseDisk(ctx, tx, baseDisk.ID)
	if err != nil {
		return err
	}

	if found.isDoomed() {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err:    fmt.Errorf("disk=%v is already deleting/deleted", found),
			Silent: true,
		}
	}

	if found.status == baseDiskStatusReady {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	updated := found
	updated.status = baseDiskStatusReady

	err = s.updateBaseDisk(ctx, tx, baseDiskTransition{
		oldState: &found,
		state:    &updated,
	})
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) baseDiskCreationFailed(
	ctx context.Context,
	session *persistence.Session,
	lookup BaseDisk,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from base_disks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(lookup.ID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	found, err := scanBaseDisk(res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if found.isDoomed() {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	if found.status == baseDiskStatusReady {
		// In fact, create was successful. Nothing to do.
		return tx.Commit(ctx)
	}

	updated := found
	updated.status = baseDiskStatusDeleting

	err = s.updateBaseDisk(ctx, tx, baseDiskTransition{
		oldState: &found,
		state:    &updated,
	})
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) getPoolConfig(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	zoneID string,
) (*poolConfig, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select *
		from configs
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return nil, res.Err()
	}

	config, err := scanConfig(res)
	if err != nil {
		return &config, err
	}

	return &config, res.Err()
}

func (s *storageYDB) getPoolConfigs(
	ctx context.Context,
	session *persistence.Session,
) ([]poolConfig, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";

		select *
		from configs
	`, s.tablesPath), ydb_table.NewQueryParameters())
	if err != nil {
		return nil, err
	}
	defer res.Close()

	m := make(map[string]*poolConfig)

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			config, err := scanConfig(res)
			if err != nil {
				return nil, err
			}

			key := config.imageID + config.zoneID

			if c, ok := m[key]; ok {
				// Just accumulate capacities of all pool kinds.
				// TODO: Do something better.
				config.capacity += c.capacity
			}
			m[key] = &config
		}
	}

	configs := make([]poolConfig, 0, len(m))
	for _, config := range m {
		configs = append(configs, *config)
	}

	return configs, res.Err()
}

func (s *storageYDB) getBaseDisksScheduling(
	ctx context.Context,
	session *persistence.Session,
) ([]BaseDisk, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";

		select *
		from scheduling
	`, s.tablesPath), ydb_table.NewQueryParameters())
	if err != nil {
		return nil, err
	}
	defer res.Close()

	var ids []string
	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var id string
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("base_disk_id", &id),
			)
			if err != nil {
				return nil, &errors.NonRetriableError{
					Err: fmt.Errorf("getBaseDisksScheduling: failed to parse row: %w", err),
				}
			}

			ids = append(ids, id)
		}
	}

	found, err := s.findBaseDisks(ctx, session, ids)
	if err != nil {
		return nil, err
	}

	var baseDisks []BaseDisk
	for _, baseDisk := range found {
		baseDisks = append(baseDisks, baseDisk.toBaseDisk())
	}

	return baseDisks, res.Err()
}

func (s *storageYDB) takeBaseDisksToScheduleForPool(
	ctx context.Context,
	session *persistence.Session,
	config poolConfig,
) ([]BaseDisk, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return nil, err
	}
	defer tx.Rollback(ctx)

	// Check that config is still present (NBS-2171).
	configured, err := s.isPoolConfiguredTx(
		ctx,
		tx,
		config.imageID,
		config.zoneID,
	)
	if err != nil {
		return nil, err
	}

	if !configured {
		// Nothing to schedule.
		return nil, tx.Commit(ctx)
	}

	pool, err := s.getPoolOrDefault(ctx, tx, config.imageID, config.zoneID)
	if err != nil {
		return nil, err
	}

	pool.status = poolStatusReady

	if pool.size >= config.capacity {
		// Pool is already full.
		return nil, tx.Commit(ctx)
	}

	baseDiskTemplate := s.generateBaseDiskForPool(
		config.imageID,
		config.zoneID,
		config.imageSize,
		nil, // srcDisk
		"",  // srcDiskCheckpointID
		0,   // srcDiskCheckpointSize
	)

	// size and capacity are measured in slots, not base disks.
	wantToCreate := config.capacity - pool.size
	// TODO: introduce new capacity param that is measured in units
	wantToCreate = divideWithRoundingUp(
		wantToCreate,
		baseDiskTemplate.freeSlots(),
	)

	if s.maxBaseDisksInflight <= pool.baseDisksInflight {
		// Limit number of base disks being currently created.
		return nil, tx.Commit(ctx)
	}

	maxPermittedToCreate := s.maxBaseDisksInflight - pool.baseDisksInflight
	willCreate := min(maxPermittedToCreate, wantToCreate)

	pool.size += willCreate * baseDiskTemplate.freeSlots()
	pool.freeUnits += willCreate * baseDiskTemplate.freeUnits()
	pool.baseDisksInflight += willCreate

	var newBaseDisks []baseDisk
	for i := uint64(0); i < willCreate; i++ {
		// All base disks should have the same params.
		baseDiskTemplate.id = generateDiskID()
		newBaseDisks = append(newBaseDisks, baseDiskTemplate)

		logging.Info(
			ctx,
			"takeBaseDisksToScheduleForPool generated base disk: %v",
			baseDiskTemplate,
		)
	}

	transitions := make([]baseDiskTransition, 0)
	for i := 0; i < len(newBaseDisks); i++ {
		transitions = append(transitions, baseDiskTransition{
			oldState: nil,
			state:    &newBaseDisks[i],
		})
	}

	err = s.updateSchedulingTable(ctx, tx, transitions)
	if err != nil {
		return nil, err
	}

	err = s.updateFreeTable(ctx, tx, transitions)
	if err != nil {
		return nil, err
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $pool as List<%v>;

		upsert into pools
		select *
		from AS_TABLE($pool)
	`, s.tablesPath, poolStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$pool", ydb_types.ListValue(pool.structValue())),
	))
	if err != nil {
		return nil, err
	}

	values := make([]ydb_types.Value, 0, len(newBaseDisks))
	for _, baseDisk := range newBaseDisks {
		values = append(values, baseDisk.structValue())
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $base_disks as List<%v>;

		upsert into base_disks
		select *
		from AS_TABLE($base_disks)
	`, s.tablesPath, baseDiskStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$base_disks", ydb_types.ListValue(values...)),
	))
	if err != nil {
		return nil, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return nil, err
	}

	var res []BaseDisk
	for _, baseDisk := range newBaseDisks {
		res = append(res, baseDisk.toBaseDisk())
	}

	return res, nil
}

func (s *storageYDB) takeBaseDisksToSchedule(
	ctx context.Context,
	session *persistence.Session,
) ([]BaseDisk, error) {

	configs, err := s.getPoolConfigs(ctx, session)
	if err != nil {
		return nil, err
	}

	scheduling, err := s.getBaseDisksScheduling(ctx, session)
	if err != nil {
		return nil, err
	}

	var baseDisks []BaseDisk

	for _, disk := range scheduling {
		if disk.SrcDisk != nil {
			baseDisks = append(baseDisks, disk)
		}
	}

	var toSchedule []poolConfig

	for _, config := range configs {
		alreadyScheduling := false

		for _, disk := range scheduling {
			if disk.ImageID == config.imageID && disk.ZoneID == config.zoneID {
				alreadyScheduling = true
				baseDisks = append(baseDisks, disk)
			}
		}

		if alreadyScheduling {
			// We have something to schedule, so skip this pool.
			continue
		}

		toSchedule = append(toSchedule, config)
	}

	startIndex := 0
	for startIndex < len(toSchedule) {
		endIndex := startIndex + s.takeBaseDisksToScheduleParallelism
		if endIndex > len(toSchedule) {
			endIndex = len(toSchedule)
		}

		group, ctx := errgroup.WithContext(ctx)
		c := make(chan []BaseDisk)

		for i := startIndex; i < endIndex; i++ {
			config := toSchedule[i]

			group.Go(func() error {
				return s.db.Execute(
					ctx,
					func(ctx context.Context, session *persistence.Session) (err error) {
						var disks []BaseDisk
						disks, err = s.takeBaseDisksToScheduleForPool(
							ctx,
							session,
							config,
						)
						if err != nil {
							return err
						}

						select {
						case c <- disks:
						case <-ctx.Done():
							return ctx.Err()
						}
						return nil
					},
				)
			})
		}
		go func() {
			_ = group.Wait()
			close(c)
		}()

		for disks := range c {
			baseDisks = append(baseDisks, disks...)
		}

		err := group.Wait()
		if err != nil {
			return nil, err
		}

		startIndex = endIndex
	}

	return baseDisks, nil
}

func (s *storageYDB) getReadyPoolInfos(
	ctx context.Context,
	session *persistence.Session,
) ([]PoolInfo, error) {

	configs, err := s.getPoolConfigs(ctx, session)
	if err != nil {
		return nil, err
	}

	var poolInfos []PoolInfo
	for _, config := range configs {
		err := func() error {
			_, res, err := session.ExecuteRO(
				ctx,
				s.makeSelectPoolsQuery(),
				ydb_table.NewQueryParameters(
					ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(config.imageID)),
					ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(config.zoneID)),
				),
			)
			if err != nil {
				return err
			}
			defer res.Close()

			if !res.NextResultSet(ctx) || !res.NextRow() {
				return nil
			}
			pool, err := scanPool(res)

			if err != nil {
				return err
			}

			if pool.status != poolStatusReady {
				return nil
			}

			poolInfos = append(poolInfos, PoolInfo{
				ImageID:       config.imageID,
				ZoneID:        config.zoneID,
				FreeUnits:     pool.freeUnits,
				AcquiredUnits: pool.acquiredUnits,
				Capacity:      uint32(config.capacity),
				ImageSize:     config.imageSize,
				CreatedAt:     pool.createdAt,
			})

			return nil
		}()

		if err != nil {
			return nil, err
		}
	}

	return poolInfos, nil
}

func (s *storageYDB) baseDiskScheduled(
	ctx context.Context,
	session *persistence.Session,
	baseDisk BaseDisk,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	found, err := s.getBaseDisk(ctx, tx, baseDisk.ID)
	if err != nil {
		return err
	}

	if found.status != baseDiskStatusScheduling {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	updated := found
	updated.createTaskID = baseDisk.CreateTaskID
	updated.status = baseDiskStatusCreating

	err = s.updateBaseDisk(ctx, tx, baseDiskTransition{
		oldState: &found,
		state:    &updated,
	})
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) baseDisksScheduled(
	ctx context.Context,
	session *persistence.Session,
	baseDisks []BaseDisk,
) error {

	for _, disk := range baseDisks {
		err := s.baseDiskScheduled(ctx, session, disk)
		if err != nil {
			return err
		}
	}

	return nil
}

func (s *storageYDB) configurePool(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	zoneID string,
	capacity uint32,
	imageSize uint64,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	p, err := s.getPoolOrDefault(ctx, tx, imageID, zoneID)
	if err != nil {
		return err
	}

	if p.status == poolStatusDeleted {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"can't configure already deleted pool, imageID=%v, zoneID=%v",
				imageID,
				zoneID,
			),
			Silent: true,
		}
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;
		declare $kind as Int64;
		declare $capacity as Uint64;
		declare $image_size as Uint64;

		upsert into configs (image_id, zone_id, kind, capacity, image_size)
		values ($image_id, $zone_id, $kind, $capacity, $image_size)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
		ydb_table.ValueParam("$kind", ydb_types.Int64Value(0)), // TODO: get rid of this param.
		ydb_table.ValueParam("$capacity", ydb_types.Uint64Value(uint64(capacity))),
		ydb_table.ValueParam("$image_size", ydb_types.Uint64Value(imageSize)),
	))
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) markBaseDisksDeleting(
	ctx context.Context,
	tx *persistence.Transaction,
	toDelete []baseDisk,
) error {

	for i := 0; i < len(toDelete); i++ {
		toDelete[i].status = baseDiskStatusDeleting
	}

	values := make([]ydb_types.Value, 0, len(toDelete))
	for _, disk := range toDelete {
		values = append(values, disk.structValue())
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $base_disks as List<%v>;

		upsert into base_disks
		select *
		from AS_TABLE($base_disks)
	`, s.tablesPath, baseDiskStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$base_disks", ydb_types.ListValue(values...)),
	))
	if err != nil {
		return err
	}

	values = make([]ydb_types.Value, 0, len(toDelete))
	for _, disk := range toDelete {
		structValue := ydb_types.StructValue(ydb_types.StructFieldValue(
			"base_disk_id",
			ydb_types.UTF8Value(disk.id),
		))
		values = append(values, structValue)
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<Struct<base_disk_id:Utf8>>;

		upsert into deleting
		select *
		from AS_TABLE($values)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) deletePool(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	zoneID string,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	pools, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select *
		from pools
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return err
	}
	defer pools.Close()

	if !pools.NextResultSet(ctx) || !pools.NextRow() {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	p, err := scanPool(pools)
	if err != nil {
		// Nothing to do.
		if commitErr := tx.Commit(ctx); commitErr != nil {
			return commitErr
		}

		return err
	}

	if err = pools.Err(); err != nil {
		// Nothing to do.
		if commitErr := tx.Commit(ctx); commitErr != nil {
			return commitErr
		}

		return err
	}

	if p.status == poolStatusDeleted {
		// Nothing to do.
		return tx.Commit(ctx)
	}

	if len(p.lockID) != 0 {
		err := tx.Commit(ctx)
		if err != nil {
			return err
		}

		// Pool is locked.
		return &errors.InterruptExecutionError{}
	}

	freeBaseDisks, err := s.getFreeBaseDisks(ctx, tx, imageID, zoneID)
	if err != nil {
		return err
	}

	var toDelete []baseDisk
	for _, baseDisk := range freeBaseDisks {
		if baseDisk.activeSlots == 0 {
			toDelete = append(toDelete, baseDisk)
		}
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		delete from configs where
		image_id = $image_id and zone_id = $zone_id;

		delete from free where
		image_id = $image_id and zone_id = $zone_id;

		delete from scheduling where
		image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return err
	}

	p = pool{
		imageID:           imageID,
		zoneID:            zoneID,
		size:              0,
		freeUnits:         0,
		acquiredUnits:     0,
		baseDisksInflight: 0,
		status:            poolStatusDeleted,
		createdAt:         p.createdAt,
	}
	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $pool as List<%v>;

		upsert into pools
		select *
		from AS_TABLE($pool)
	`, s.tablesPath, poolStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$pool", ydb_types.ListValue(p.structValue())),
	))
	if err != nil {
		return err
	}

	if len(toDelete) == 0 {
		return tx.Commit(ctx)
	}

	err = s.markBaseDisksDeleting(ctx, tx, toDelete)
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) imageDeleting(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	pools, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;

		select *
		from pools
		where image_id = $image_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
	))
	if err != nil {
		return err
	}
	defer pools.Close()

	toDelete := make([]baseDisk, 0)
	zones := make([]string, 0)
	createdAts := make([]time.Time, 0)

	for pools.NextResultSet(ctx) {
		for pools.NextRow() {
			p, err := scanPool(pools)
			if err != nil {
				return err
			}

			if p.status == poolStatusDeleted {
				// Nothing to do.
				continue
			}

			if len(p.lockID) != 0 {
				err := tx.Commit(ctx)
				if err != nil {
					return err
				}

				// Pool is locked.
				return &errors.InterruptExecutionError{}
			}

			zones = append(zones, p.zoneID)
			createdAts = append(createdAts, p.createdAt)

			freeBaseDisks, err := s.getFreeBaseDisks(
				ctx,
				tx,
				imageID,
				p.zoneID,
			)
			if err != nil {
				return err
			}

			for _, baseDisk := range freeBaseDisks {
				if baseDisk.activeSlots == 0 {
					baseDisk.status = baseDiskStatusDeleting
					toDelete = append(toDelete, baseDisk)
				}
			}
		}
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;

		delete from configs where
		image_id = $image_id;

		delete from free where
		image_id = $image_id;

		delete from scheduling where
		image_id = $image_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
	))
	if err != nil {
		return err
	}

	// TODO: Use one query for all zones.
	for i, zoneID := range zones {
		pool := pool{
			imageID:           imageID,
			zoneID:            zoneID,
			size:              0,
			freeUnits:         0,
			acquiredUnits:     0,
			baseDisksInflight: 0,
			status:            poolStatusDeleted,
			createdAt:         createdAts[i],
		}
		_, err = tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $pool as List<%v>;

			upsert into pools
			select *
			from AS_TABLE($pool)
		`, s.tablesPath, poolStructTypeString()), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$pool", ydb_types.ListValue(pool.structValue())),
		))
		if err != nil {
			return err
		}
	}

	if len(toDelete) == 0 {
		return tx.Commit(ctx)
	}

	err = s.markBaseDisksDeleting(ctx, tx, toDelete)
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) getBaseDisksToDelete(
	ctx context.Context,
	session *persistence.Session,
	limit uint64,
) (baseDisks []BaseDisk, err error) {

	_, deleting, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $limit as Uint64;

		select *
		from deleting
		limit $limit
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(limit)),
	))
	if err != nil {
		return nil, err
	}
	defer deleting.Close()

	for deleting.NextResultSet(ctx) {
		for deleting.NextRow() {
			var (
				baseDiskID string
				res        ydb_result.Result
			)
			err = deleting.ScanNamed(
				ydb_named.OptionalWithDefault("base_disk_id", &baseDiskID),
			)
			if err != nil {
				return nil, &errors.NonRetriableError{
					Err: fmt.Errorf("getBaseDisksToDelete: failed to parse row: %w", err),
				}
			}

			_, res, err = session.ExecuteRO(ctx, fmt.Sprintf(`
				--!syntax_v1
				pragma TablePathPrefix = "%v";
				declare $id as Utf8;
				declare $status as Int64;

				select *
				from base_disks
				where id = $id and status = $status
			`, s.tablesPath), ydb_table.NewQueryParameters(
				ydb_table.ValueParam("$id", ydb_types.UTF8Value(baseDiskID)),
				ydb_table.ValueParam("$status", ydb_types.Int64Value(int64(baseDiskStatusDeleting))),
			))
			if err != nil {
				return nil, err
			}
			defer res.Close() // TODO: dont use defer in loop!!!

			if !res.NextResultSet(ctx) || !res.NextRow() {
				return nil, res.Err()
			}

			var baseDisk baseDisk
			baseDisk, err := scanBaseDisk(res)
			if err != nil {
				return nil, err
			}

			baseDisks = append(baseDisks, baseDisk.toBaseDisk())
		}
	}

	return baseDisks, deleting.Err()
}

func (s *storageYDB) baseDiskDeleted(
	ctx context.Context,
	session *persistence.Session,
	lookup BaseDisk,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from base_disks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(lookup.ID)),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	found, err := scanBaseDisk(res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}

		return err
	}

	if found.status == baseDiskStatusDeleted {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	if found.status != baseDiskStatusDeleting {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"internal inconsistency: base disk has invalid status=%v",
				found.status,
			),
		}
	}

	updated := found
	updated.deletedAt = time.Now()
	updated.status = baseDiskStatusDeleted

	err = s.updateBaseDisk(ctx, tx, baseDiskTransition{
		oldState: &found,
		state:    &updated,
	})
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}

func (s *storageYDB) baseDisksDeleted(
	ctx context.Context,
	session *persistence.Session,
	baseDisks []BaseDisk,
) error {

	for _, disk := range baseDisks {
		err := s.baseDiskDeleted(ctx, session, disk)
		if err != nil {
			return err
		}
	}

	return nil
}

func (s *storageYDB) clearDeletedBaseDisks(
	ctx context.Context,
	session *persistence.Session,
	deletedBefore time.Time,
	limit int,
) error {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $deleted_before as Timestamp;
		declare $limit as Uint64;

		select *
		from deleted
		where deleted_at < $deleted_before
		limit $limit
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$deleted_before", persistence.TimestampValue(deletedBefore)),
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(uint64(limit))),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var (
				deletedAt  time.Time
				baseDiskID string
			)
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("deleted_at", &deletedAt),
				ydb_named.OptionalWithDefault("base_disk_id", &baseDiskID),
			)
			if err != nil {
				return &errors.NonRetriableError{
					Err: fmt.Errorf(
						"clearDeletedBaseDisks: failed to parse row: %w",
						err,
					),
				}
			}
			_, _, err := session.ExecuteRW(ctx, fmt.Sprintf(`
				--!syntax_v1
				pragma TablePathPrefix = "%v";
				declare $deleted_at as Timestamp;
				declare $base_disk_id as Utf8;
				declare $status as Int64;

				delete from base_disks
				where id = $base_disk_id and status = $status;

				delete from deleted
				where deleted_at = $deleted_at and base_disk_id = $base_disk_id
			`, s.tablesPath), ydb_table.NewQueryParameters(
				ydb_table.ValueParam("$deleted_at", persistence.TimestampValue(deletedAt)),
				ydb_table.ValueParam("$base_disk_id", ydb_types.UTF8Value(baseDiskID)),
				ydb_table.ValueParam("$status", ydb_types.Int64Value(int64(baseDiskStatusDeleted))),
			))
			if err != nil {
				return err
			}
		}
	}

	return res.Err()
}

func (s *storageYDB) clearReleasedSlots(
	ctx context.Context,
	session *persistence.Session,
	releasedBefore time.Time,
	limit int,
) error {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $released_before as Timestamp;
		declare $limit as Uint64;

		select *
		from released
		where released_at < $released_before
		limit $limit
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$released_before", persistence.TimestampValue(releasedBefore)),
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(uint64(limit))),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var (
				releasedAt    time.Time
				overlayDiskID string
			)
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("released_at", &releasedAt),
				ydb_named.OptionalWithDefault("overlay_disk_id", &overlayDiskID),
			)
			if err != nil {
				return &errors.NonRetriableError{
					Err: fmt.Errorf("clearReleasedSlots: failed to parse row: %w", err),
				}
			}

			_, _, err := session.ExecuteRW(ctx, fmt.Sprintf(`
				--!syntax_v1
				pragma TablePathPrefix = "%v";
				declare $released_at as Timestamp;
				declare $overlay_disk_id as Utf8;
				declare $status as Int64;

				delete from slots
				where overlay_disk_id = $overlay_disk_id and status = $status;

				delete from released
				where released_at = $released_at and overlay_disk_id = $overlay_disk_id
			`, s.tablesPath), ydb_table.NewQueryParameters(
				ydb_table.ValueParam("$released_at", persistence.TimestampValue(releasedAt)),
				ydb_table.ValueParam("$overlay_disk_id", ydb_types.UTF8Value(overlayDiskID)),
				ydb_table.ValueParam("$status", ydb_types.Int64Value(int64(slotStatusReleased))),
			))
			if err != nil {
				return err
			}
		}
	}

	return res.Err()
}

func (s *storageYDB) getAcquiredSlots(
	ctx context.Context,
	tx *persistence.Transaction,
	baseDiskID string,
) ([]slot, error) {

	read, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $base_disk_id as Utf8;

		select overlay_disk_id
		from overlay_disk_ids
		where base_disk_id = $base_disk_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$base_disk_id", ydb_types.UTF8Value(baseDiskID)),
	))
	if err != nil {
		return nil, err
	}
	defer read.Close()

	overlayDiskIDs := make([]ydb_types.Value, 0)
	for read.NextResultSet(ctx) {
		for read.NextRow() {
			var id string
			err = read.ScanNamed(
				ydb_named.OptionalWithDefault("overlay_disk_id", &id),
			)
			if err != nil {
				return nil, &errors.NonRetriableError{
					Err: fmt.Errorf("getAcquiredSlots: failed to parse row: %w", err),
				}
			}

			overlayDiskIDs = append(overlayDiskIDs, ydb_types.UTF8Value(id))
		}
	}

	if len(overlayDiskIDs) == 0 {
		return nil, nil
	}

	read, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $overlay_disk_ids as List<Utf8>;

		select *
		from slots
		where overlay_disk_id in $overlay_disk_ids
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$overlay_disk_ids", ydb_types.ListValue(overlayDiskIDs...)),
	))
	if err != nil {
		return nil, err
	}
	defer read.Close()

	slots := make([]slot, 0)

	for read.NextResultSet(ctx) {
		for read.NextRow() {
			slot, err := scanSlot(read)
			if err != nil {
				commitErr := tx.Commit(ctx)
				if commitErr != nil {
					return nil, commitErr
				}

				return nil, err
			}
			slots = append(slots, slot)
		}
	}

	return slots, nil
}

func (s *storageYDB) getFreeBaseDisks(
	ctx context.Context,
	tx *persistence.Transaction,
	imageID string,
	zoneID string,
) ([]baseDisk, error) {

	// TODO: use join here.
	read, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select *
		from free
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return nil, err
	}
	defer read.Close()

	var ids []string
	for read.NextResultSet(ctx) {
		for read.NextRow() {
			var id string
			err = read.ScanNamed(
				ydb_named.OptionalWithDefault("base_disk_id", &id),
			)
			if err != nil {
				return nil, &errors.NonRetriableError{
					Err: fmt.Errorf("getFreeBaseDisks: failed to parse row: %w", err),
				}
			}

			ids = append(ids, id)
		}
	}
	if err = read.Err(); err != nil {
		return nil, err
	}

	return s.findBaseDisksTx(ctx, tx, ids)
}

func (s *storageYDB) retireBaseDisk(
	ctx context.Context,
	session *persistence.Session,
	baseDiskID string,
	srcDisk *types.Disk,
	srcDiskCheckpointID string,
	srcDiskCheckpointSize uint64,
) ([]RebaseInfo, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return nil, err
	}
	defer tx.Rollback(ctx)

	baseDisk, err := s.findBaseDisk(ctx, tx, baseDiskID)
	if err != nil {
		return nil, err
	}

	if baseDisk == nil || baseDisk.isDoomed() {
		// Already retired.
		return nil, tx.Commit(ctx)
	}

	slots, err := s.getAcquiredSlots(ctx, tx, baseDiskID)
	if err != nil {
		return nil, err
	}

	logging.Debug(
		ctx,
		"retireBaseDisk: baseDiskID=%v, slots=%v",
		baseDiskID,
		slots,
	)

	slotTransitions := make([]slotTransition, 0)

	for i := 0; i < len(slots); i++ {
		oldState := slots[i]
		slotTransitions = append(slotTransitions, slotTransition{
			oldState: &oldState,
			state:    &slots[i],
		})
	}

	imageID := baseDisk.imageID
	zoneID := baseDisk.zoneID
	imageSize := baseDisk.imageSize

	config, err := s.getPoolConfig(ctx, session, imageID, zoneID)
	if err != nil {
		return nil, err
	}

	if config != nil {
		imageSize = config.imageSize
	}

	freeBaseDisks, err := s.getFreeBaseDisks(ctx, tx, imageID, zoneID)
	if err != nil {
		return nil, err
	}

	baseDiskTransitions := make([]baseDiskTransition, 0)

	for i := 0; i < len(freeBaseDisks); i++ {
		oldState := freeBaseDisks[i]
		baseDiskTransitions = append(baseDiskTransitions, baseDiskTransition{
			oldState: &oldState,
			state:    &freeBaseDisks[i],
		})
	}

	rebaseInfos := make([]RebaseInfo, 0)
	slotIndex := 0
	baseDiskIndex := 0

	for slotIndex < len(slotTransitions) {
		slot := slotTransitions[slotIndex].state

		if len(slot.targetBaseDiskID) != 0 {
			// Should be idempotent.
			rebaseInfos = append(rebaseInfos, RebaseInfo{
				OverlayDisk: &types.Disk{
					ZoneId: slot.zoneID,
					DiskId: slot.overlayDiskID,
				},
				BaseDiskID:       slot.baseDiskID,
				TargetBaseDiskID: slot.targetBaseDiskID,
				SlotGeneration:   slot.generation,
			})

			slotIndex++
			continue
		}

		if baseDiskIndex >= len(baseDiskTransitions) {
			baseDisk := s.generateBaseDiskForPool(
				imageID,
				zoneID,
				imageSize,
				srcDisk,
				srcDiskCheckpointID,
				srcDiskCheckpointSize,
			)
			logging.Info(
				ctx,
				"retireBaseDisk generated base disk: %v",
				baseDisk,
			)

			baseDiskTransitions = append(baseDiskTransitions, baseDiskTransition{
				oldState: nil,
				state:    &baseDisk,
			})
			baseDiskIndex = len(baseDiskTransitions) - 1
		}

		target := baseDiskTransitions[baseDiskIndex].state

		if !target.hasFreeSlots() {
			baseDiskIndex++
			continue
		}

		if target.id == baseDiskID {
			baseDiskIndex++
			continue
		}

		if target.retiring {
			baseDiskIndex++
			continue
		}

		err := acquireTargetUnitsAndSlots(ctx, tx, target, slot)
		if err != nil {
			return nil, err
		}

		slot.targetBaseDiskID = target.id
		slot.generation += 1

		rebaseInfos = append(rebaseInfos, RebaseInfo{
			OverlayDisk: &types.Disk{
				ZoneId: slot.zoneID,
				DiskId: slot.overlayDiskID,
			},
			BaseDiskID:       slot.baseDiskID,
			TargetBaseDiskID: slot.targetBaseDiskID,
			SlotGeneration:   slot.generation,
		})

		slotIndex++
	}

	baseDiskOldState := *baseDisk
	// Remove base disk from pool. It also removes base disk from 'free' table.
	baseDisk.fromPool = false
	baseDisk.retiring = true

	baseDiskTransitions = append(baseDiskTransitions, baseDiskTransition{
		oldState: &baseDiskOldState,
		state:    baseDisk,
	})

	err = s.updateBaseDisksAndSlots(ctx, tx, baseDiskTransitions, slotTransitions)
	if err != nil {
		return nil, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return nil, err
	}

	logging.Debug(
		ctx,
		"retireBaseDisk: baseDisk=%v, rebaseInfos=%v",
		baseDisk,
		rebaseInfos,
	)
	return rebaseInfos, nil
}

func (s *storageYDB) isBaseDiskRetired(
	ctx context.Context,
	session *persistence.Session,
	baseDiskID string,
) (bool, error) {

	_, read, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from base_disks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(baseDiskID)),
	))
	if err != nil {
		return false, err
	}
	defer read.Close()

	if !read.NextResultSet(ctx) || !read.NextRow() {
		return true, read.Err()
	}

	baseDisk, err := scanBaseDisk(read)
	if err != nil {
		return false, err
	}

	retired := baseDisk.isDoomed()
	if !retired {
		logging.Debug(ctx, "baseDisk=%v is not retired", baseDisk)
	}

	return retired, nil
}

func (s *storageYDB) listBaseDisks(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	zoneID string,
) ([]BaseDisk, error) {

	// TODO: should we limit scan here?
	_, read, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;
		declare $ready as Int64;

		select *
		from base_disks
		where image_id = $image_id and zone_id = $zone_id and status = $ready
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
		ydb_table.ValueParam("$ready", ydb_types.Int64Value(int64(baseDiskStatusReady))),
	))
	if err != nil {
		return nil, err
	}
	defer read.Close()

	baseDisks := make([]BaseDisk, 0)

	for read.NextResultSet(ctx) {
		for read.NextRow() {
			baseDisk, err := scanBaseDisk(read)
			if err != nil {
				return nil, err
			}

			baseDisks = append(baseDisks, baseDisk.toBaseDisk())
		}
	}

	return baseDisks, nil
}

func (s *storageYDB) lockPool(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	zoneID string,
	lockID string,
) (bool, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return false, err
	}
	defer tx.Rollback(ctx)

	read, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select *
		from pools
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return false, err
	}
	defer read.Close()

	if !read.NextResultSet(ctx) || !read.NextRow() {
		if commitErr := tx.Commit(ctx); commitErr != nil {
			return false, commitErr
		}

		return false, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"pool is not found, imageID=%v, zoneID=%v",
				imageID,
				zoneID,
			),
		}
	}
	pool, err := scanPool(read)
	if err != nil {
		if commitErr := tx.Commit(ctx); commitErr != nil {
			return false, commitErr
		}

		return false, err
	}

	if pool.status == poolStatusDeleted {
		err := tx.Commit(ctx)
		if err != nil {
			return false, err
		}

		return false, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"pool is deleted, pool=%v",
				pool,
			),
		}
	}

	if len(pool.lockID) != 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return false, err
		}

		// Should be idempotent.
		return pool.lockID == lockID, nil
	}

	pool.lockID = lockID

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $pools as List<%v>;

		upsert into pools
		select *
		from AS_TABLE($pools)
	`, s.tablesPath, poolStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$pools", ydb_types.ListValue(pool.structValue())),
	))
	if err != nil {
		return false, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return false, err
	}

	return true, nil
}

func (s *storageYDB) unlockPool(
	ctx context.Context,
	session *persistence.Session,
	imageID string,
	zoneID string,
	lockID string,
) error {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	read, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $image_id as Utf8;
		declare $zone_id as Utf8;

		select *
		from pools
		where image_id = $image_id and zone_id = $zone_id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$image_id", ydb_types.UTF8Value(imageID)),
		ydb_table.ValueParam("$zone_id", ydb_types.UTF8Value(zoneID)),
	))
	if err != nil {
		return err
	}
	defer read.Close()

	if !read.NextResultSet(ctx) || !read.NextRow() {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	pool, err := scanPool(read)
	if err != nil {
		if commitErr := tx.Commit(ctx); commitErr != nil {
			return commitErr
		}

		return err
	}

	if pool.status == poolStatusDeleted {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	if len(pool.lockID) == 0 {
		// Should be idempotent.
		return tx.Commit(ctx)
	}

	if pool.lockID != lockID {
		err := tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf(
				"failed to unlock, another lock is in progress, lockID=%v",
				pool.lockID,
			),
		}
	}

	pool.lockID = ""

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $pools as List<%v>;

		upsert into pools
		select *
		from AS_TABLE($pools)
	`, s.tablesPath, poolStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$pools", ydb_types.ListValue(pool.structValue())),
	))
	if err != nil {
		return err
	}

	return tx.Commit(ctx)
}
