package storage

import (
	"context"
	"fmt"
	"time"

	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	grpc_codes "google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type stateTransition struct {
	lastState *TaskState
	newState  TaskState
}

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) getTaskID(
	ctx context.Context,
	session *persistence.Session,
	idempotencyKey string,
	accountID string,
) (string, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return "", err
	}

	defer tx.Rollback(ctx)

	// TODO: Return account_id check when NBS-1419 is resolved.
	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $idempotency_key as Utf8;

		select task_id
		from task_ids
		where idempotency_key = $idempotency_key
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$idempotency_key", ydb_types.UTF8Value(idempotencyKey)),
	))
	if err != nil {
		return "", err
	}

	defer res.Close()

	if res.NextResultSet(ctx) && res.NextRow() {
		var id string
		err = res.ScanNamed(
			ydb_named.OptionalWithDefault("task_id", &id),
		)
		if err != nil {
			return "", &errors.NonRetriableError{
				Err: fmt.Errorf("getTaskID: failed to parse row: %w", err),
			}
		}

		if err = res.Err(); err != nil {
			return "", err
		}

		err = tx.Commit(ctx)
		if err != nil {
			return "", err
		}

		logging.Debug(
			ctx,
			"Existing taskID=%v for idempotency_key=%v account_id=%v",
			id,
			idempotencyKey,
			accountID,
		)
		return id, nil
	}

	id := generateTaskID()

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $task_id as Utf8;
		declare $idempotency_key as Utf8;
		declare $account_id as Utf8;

		upsert into task_ids
			(task_id,
			idempotency_key,
			account_id)
		values
			($task_id,
			 $idempotency_key,
			 $account_id)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$task_id", ydb_types.UTF8Value(id)),
		ydb_table.ValueParam("$idempotency_key", ydb_types.UTF8Value(idempotencyKey)),
		ydb_table.ValueParam("$account_id", ydb_types.UTF8Value(accountID)),
	))
	if err != nil {
		return "", err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return "", err
	}

	logging.Info(
		ctx,
		"New taskID=%v for idempotency_key=%v account_id=%v",
		id,
		idempotencyKey,
		accountID,
	)

	return id, nil
}

func (s *storageYDB) deleteFromTable(
	ctx context.Context,
	tx *persistence.Transaction,
	tableName string,
	id string,
) error {

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		delete from %v
		where id = $id
	`, s.tablesPath, tableName), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(id)),
	))
	return err
}

func (s *storageYDB) updateReadyToExecute(
	ctx context.Context,
	tx *persistence.Transaction,
	tableName string,
	status TaskStatus,
	transitions []stateTransition,
) error {

	values := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.lastState != nil &&
			t.lastState.Status != t.newState.Status &&
			t.lastState.Status == status {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("id", ydb_types.UTF8Value(t.lastState.ID)),
			))
		}
	}

	if len(values) != 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $values as List<Struct<id: Utf8>>;

			delete from %v on
			select *
			from AS_TABLE($values)
		`, s.tablesPath, tableName), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
		))
		if err != nil {
			return err
		}
	}

	values = make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.newState.Status != status {
			// Table is not affected by transition.
			continue
		}

		if t.lastState != nil &&
			t.lastState.Status == t.newState.Status &&
			t.lastState.GenerationID == t.newState.GenerationID {

			// Nothing to update.
			continue
		}

		values = append(values, ydb_types.StructValue(
			ydb_types.StructFieldValue("id", ydb_types.UTF8Value(t.newState.ID)),
			ydb_types.StructFieldValue("generation_id", ydb_types.Uint64Value(t.newState.GenerationID)),
			ydb_types.StructFieldValue("task_type", ydb_types.UTF8Value(t.newState.TaskType)),
			ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(t.newState.ZoneID)),
		))
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<%v>;

		upsert into %v
		select *
		from AS_TABLE($values)
	`, s.tablesPath, readyToExecuteStructTypeString(), tableName), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) updateExecuting(
	ctx context.Context,
	tx *persistence.Transaction,
	tableName string,
	status TaskStatus,
	transitions []stateTransition,
) error {

	values := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.lastState != nil &&
			t.lastState.Status != t.newState.Status &&
			t.lastState.Status == status {

			values = append(values, ydb_types.StructValue(
				ydb_types.StructFieldValue("id", ydb_types.UTF8Value(t.lastState.ID)),
			))
		}
	}

	if len(values) != 0 {
		_, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $values as List<Struct<id: Utf8>>;

			delete from %v on
			select *
			from AS_TABLE($values)
		`, s.tablesPath, tableName), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
		))
		if err != nil {
			return err
		}
	}

	values = make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.newState.Status != status {
			// Table is not affected by transition.
			continue
		}

		if t.lastState != nil &&
			t.lastState.Status == t.newState.Status &&
			t.lastState.GenerationID == t.newState.GenerationID &&
			t.lastState.ModifiedAt == t.newState.ModifiedAt {

			// Nothing to update.
			continue
		}

		values = append(values, ydb_types.StructValue(
			ydb_types.StructFieldValue("id", ydb_types.UTF8Value(t.newState.ID)),
			ydb_types.StructFieldValue("generation_id", ydb_types.Uint64Value(t.newState.GenerationID)),
			ydb_types.StructFieldValue("modified_at", persistence.TimestampValue(t.newState.ModifiedAt)),
			ydb_types.StructFieldValue("task_type", ydb_types.UTF8Value(t.newState.TaskType)),
			ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(t.newState.ZoneID)),
		))
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<%v>;

		upsert into %v
		select *
		from AS_TABLE($values)
	`, s.tablesPath, executingStructTypeString(), tableName), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) updateEnded(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []stateTransition,
) error {

	// NOTE: deletion from 'ended' table is forbidden.

	values := make([]ydb_types.Value, 0)

	for _, t := range transitions {
		if t.newState.Status != TaskStatusFinished && t.newState.Status != TaskStatusCancelled {
			// Table is not affected by transition.
			continue
		}

		if t.lastState != nil &&
			t.lastState.Status == t.newState.Status {
			// Nothing to update.
			continue
		}

		values = append(values, ydb_types.StructValue(
			ydb_types.StructFieldValue("ended_at", persistence.TimestampValue(t.newState.EndedAt)),
			ydb_types.StructFieldValue("id", ydb_types.UTF8Value(t.newState.ID)),
			ydb_types.StructFieldValue("idempotency_key", ydb_types.UTF8Value(t.newState.IdempotencyKey)),
			ydb_types.StructFieldValue("account_id", ydb_types.UTF8Value(t.newState.AccountID)),
		))
	}

	if len(values) == 0 {
		return nil
	}

	_, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<Struct<ended_at: Timestamp, id: Utf8, idempotency_key: Utf8, account_id: Utf8>>;

		upsert into ended
		select *
		from AS_TABLE($values)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) updateTaskStates(
	ctx context.Context,
	tx *persistence.Transaction,
	transitions []stateTransition,
) error {

	err := s.updateReadyToExecute(
		ctx,
		tx,
		"ready_to_run",
		TaskStatusReadyToRun,
		transitions,
	)
	if err != nil {
		return err
	}

	err = s.updateExecuting(
		ctx,
		tx,
		"running",
		TaskStatusRunning,
		transitions,
	)
	if err != nil {
		return err
	}

	err = s.updateReadyToExecute(
		ctx,
		tx,
		"ready_to_cancel",
		TaskStatusReadyToCancel,
		transitions,
	)
	if err != nil {
		return err
	}

	err = s.updateExecuting(
		ctx,
		tx,
		"cancelling",
		TaskStatusCancelling,
		transitions,
	)
	if err != nil {
		return err
	}

	err = s.updateEnded(ctx, tx, transitions)
	if err != nil {
		return err
	}

	values := make([]ydb_types.Value, 0, len(transitions))
	for _, t := range transitions {
		values = append(values, t.newState.structValue())
	}

	if len(values) == 0 {
		return nil
	}

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $values as List<%v>;

		upsert into tasks
		select *
		from AS_TABLE($values)
	`, s.tablesPath, taskStateStructTypeString()), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$values", ydb_types.ListValue(values...)),
	))
	return err
}

func (s *storageYDB) prepareUnfinishedDependencies(
	ctx context.Context,
	tx *persistence.Transaction,
	state *TaskState,
) ([]stateTransition, error) {

	transitions := make([]stateTransition, 0)

	dependencyIDs := state.Dependencies.List()
	for _, id := range dependencyIDs {
		res, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $id as Utf8;
			declare $status as Int64;

			select *
			from tasks
			where id = $id and status < $status
		`, s.tablesPath),
			ydb_table.NewQueryParameters(
				ydb_table.ValueParam("$id", ydb_types.UTF8Value(id)),
				ydb_table.ValueParam("$status", ydb_types.Int64Value(int64(TaskStatusFinished))),
			))
		if err != nil {
			return []stateTransition{}, err
		}

		defer res.Close()

		dependencies, err := s.scanTaskStates(ctx, res)
		if err != nil {
			commitErr := tx.Commit(ctx)
			if commitErr != nil {
				return []stateTransition{}, commitErr
			}

			return []stateTransition{}, err
		}

		if len(dependencies) == 0 {
			state.Dependencies.Remove(id)
		} else {
			newState := dependencies[0].DeepCopy()
			newState.dependants.Add(state.ID)

			transitions = append(transitions, stateTransition{
				lastState: &dependencies[0],
				newState:  newState,
			})
		}
	}

	return transitions, nil
}

func (s *storageYDB) prepareCreateTask(
	ctx context.Context,
	tx *persistence.Transaction,
	state *TaskState,
) ([]stateTransition, error) {

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select count(*)
		from tasks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(state.ID)),
	))
	if err != nil {
		return []stateTransition{}, err
	}

	defer res.Close()

	if res.NextResultSet(ctx) && res.NextRow() {
		var count uint64
		err = res.ScanWithDefaults(&count)
		if err != nil {
			return nil, &errors.NonRetriableError{
				Err: fmt.Errorf("prepareCreateTask: failed to parse row: %w", err),
			}
		}
		if count != 0 {
			logging.Debug(
				ctx,
				"%v tasks with id='%v' already exist",
				count,
				state.ID,
			)
			return []stateTransition{}, nil
		}
	}

	state.ChangedStateAt = state.CreatedAt

	transitions, err := s.prepareUnfinishedDependencies(ctx, tx, state)
	if err != nil {
		return []stateTransition{}, err
	}

	if len(transitions) != 0 {
		// Switch to "sleeping" state until dependencies are resolved.
		switch {
		case state.Status == TaskStatusReadyToRun || state.Status == TaskStatusRunning:
			state.Status = TaskStatusWaitingToRun

		case state.Status == TaskStatusReadyToCancel || state.Status == TaskStatusCancelling:
			state.Status = TaskStatusWaitingToCancel
		}
	}

	return append(transitions, stateTransition{
		lastState: nil,
		newState:  *state,
	}), nil
}

func (s *storageYDB) createTask(
	ctx context.Context,
	session *persistence.Session,
	state TaskState,
) (string, error) {

	if len(state.IdempotencyKey) == 0 {
		return "", &errors.NonRetriableError{
			Err: fmt.Errorf("failed to create task without IdempotencyKey"),
		}
	}

	// TODO: NBS-1419: Temporarily disable accountID check because we have
	// idempotency problem.
	state.AccountID = ""

	taskID, err := s.getTaskID(
		ctx,
		session,
		state.IdempotencyKey,
		state.AccountID,
	)
	if err != nil {
		return "", fmt.Errorf("failed to get task id: %w", err)
	}

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return "", err
	}
	defer tx.Rollback(ctx)

	state.ID = taskID

	transitions, err := s.prepareCreateTask(ctx, tx, &state)
	if err != nil {
		return "", err
	}

	created := false

	if len(transitions) != 0 {
		err = s.updateTaskStates(ctx, tx, transitions)
		if err != nil {
			return "", err
		}

		created = true
	}

	err = tx.Commit(ctx)
	if err != nil {
		return "", err
	}

	if created {
		s.metrics.OnTaskCreated(state, 1)
	}

	return taskID, nil
}

func (s *storageYDB) addRegularTasks(
	ctx context.Context,
	tx *persistence.Transaction,
	state TaskState,
	count int,
) error {

	var err error
	transitions := make([]stateTransition, 0, count)

	for i := 0; i < count; i++ {
		st := state
		st.ID = generateTaskID()

		var t []stateTransition
		t, err = s.prepareCreateTask(ctx, tx, &st)
		if err != nil {
			return err
		}

		transitions = append(transitions, t...)
	}

	if len(transitions) == 0 {
		return nil
	}

	return s.updateTaskStates(ctx, tx, transitions)
}

func (s *storageYDB) createRegularTasks(
	ctx context.Context,
	session *persistence.Session,
	state TaskState,
	scheduleInterval time.Duration,
	maxTasksInflight int,
) error {

	state.Regular = true

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $task_type as Utf8;

		select *
		from schedules
		where task_type = $task_type
	`, s.tablesPath),
		ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$task_type", ydb_types.UTF8Value(state.TaskType)),
		))
	if err != nil {
		return err
	}
	defer res.Close()

	found := false
	sch := schedule{}

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("task_type", &sch.taskType),
				ydb_named.OptionalWithDefault("scheduled_at", &sch.scheduledAt),
				ydb_named.OptionalWithDefault("tasks_inflight", &sch.tasksInflight),
			)
			if err != nil {
				return &errors.NonRetriableError{
					Err: fmt.Errorf("createRegularTasks: failed to parse row: %w", err),
				}
			}
			found = true
		}
	}

	if err = res.Err(); err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}
		return err
	}

	scheduled := false

	if found {
		schedulingTime := sch.scheduledAt.Add(scheduleInterval)

		if sch.tasksInflight == 0 && state.CreatedAt.After(schedulingTime) {
			err := s.addRegularTasks(ctx, tx, state, maxTasksInflight)
			if err != nil {
				return err
			}

			sch.tasksInflight = uint64(maxTasksInflight)
			scheduled = true
		}
	} else {
		err := s.addRegularTasks(ctx, tx, state, maxTasksInflight)
		if err != nil {
			return err
		}

		sch.taskType = state.TaskType
		sch.tasksInflight = uint64(maxTasksInflight)
		scheduled = true
	}

	if scheduled {
		sch.scheduledAt = state.CreatedAt

		_, err = tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $task_type as Utf8;
			declare $scheduled_at as Timestamp;
			declare $tasks_inflight as Uint64;

			upsert into schedules (task_type, scheduled_at, tasks_inflight)
			values ($task_type, $scheduled_at, $tasks_inflight)
		`, s.tablesPath), ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$task_type", ydb_types.UTF8Value(sch.taskType)),
			ydb_table.ValueParam("$scheduled_at", persistence.TimestampValue(sch.scheduledAt)),
			ydb_table.ValueParam("$tasks_inflight", ydb_types.Uint64Value(sch.tasksInflight)),
		))
		if err != nil {
			return err
		}
	}

	err = tx.Commit(ctx)
	if err != nil {
		return err
	}

	if scheduled {
		s.metrics.OnTaskCreated(state, maxTasksInflight)
	}

	return nil
}

func (s *storageYDB) getTask(
	ctx context.Context,
	session *persistence.Session,
	taskID string,
) (TaskState, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from tasks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(taskID)),
	))
	if err != nil {
		return TaskState{}, err
	}
	defer res.Close()

	tasks, err := s.scanTaskStates(ctx, res)
	if err != nil {
		return TaskState{}, fmt.Errorf("failed to scan []TaskState: %w", err)
	}

	if len(tasks) == 0 {
		return TaskState{}, &errors.NonRetriableError{
			Err: &errors.NotFoundError{TaskID: taskID},
		}
	}

	return tasks[0], nil
}

func (s *storageYDB) getTaskByIdempotencyKey(
	ctx context.Context,
	session *persistence.Session,
	idempotencyKey string,
	accountID string,
) (TaskState, error) {

	if len(idempotencyKey) == 0 {
		return TaskState{}, &errors.NonRetriableError{
			Err: fmt.Errorf("idempotencyKey should be defined"),
		}
	}

	// TODO: Return account_id check when NBS-1419 is resolved.
	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $idempotency_key as Utf8;

		select task_id
		from task_ids
		where idempotency_key = $idempotency_key
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$idempotency_key", ydb_types.UTF8Value(idempotencyKey)),
	))
	if err != nil {
		return TaskState{}, err
	}
	defer res.Close()

	if !res.NextResultSet(ctx) || !res.NextRow() {
		return TaskState{}, &errors.NonRetriableError{
			Err: &errors.NotFoundError{IdempotencyKey: idempotencyKey},
		}
	}
	var id string
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("task_id", &id),
	)
	if err != nil {
		return TaskState{}, &errors.NonRetriableError{
			Err: fmt.Errorf("getTaskByIdempotencyKey: failed to parse row: %w", err),
		}
	}
	if err = res.Err(); err != nil {
		return TaskState{}, err
	}
	return s.getTask(ctx, session, id)
}

func (s *storageYDB) listTasks(
	ctx context.Context,
	session *persistence.Session,
	tableName string,
	limit uint64,
) ([]TaskInfo, error) {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		pragma AnsiInForEmptyOrNullableItemsCollections;
		declare $limit as Uint64;
		declare $typeWhitelist as List<Utf8>;
		declare $typeBlacklist as List<Utf8>;
		declare $local_zone_id as Utf8;

		select *
		from %v
		where
			(ListLength($typeWhitelist) == 0 or task_type in $typeWhitelist) and
			(task_type not in $typeBlacklist) and
			(zone_id is null or Len(zone_id) == 0 or zone_id == $local_zone_id)
		limit $limit
	`, s.tablesPath, tableName), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(uint64(limit))),
		ydb_table.ValueParam("$typeWhitelist", strListValue(s.taskTypeWhitelist)),
		ydb_table.ValueParam("$typeBlacklist", strListValue(s.taskTypeBlacklist)),
		ydb_table.ValueParam("$local_zone_id", ydb_types.UTF8Value(s.ZoneID)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	return scanTaskInfos(ctx, res)
}

func (s *storageYDB) listTasksStallingWhileExecuting(
	ctx context.Context,
	session *persistence.Session,
	excludingHostname string,
	tableName string,
	limit uint64,
) ([]TaskInfo, error) {

	stallingTime := time.Now().Add(-s.taskStallingTimeout)
	// TODO: Use excludingHostname
	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		pragma AnsiInForEmptyOrNullableItemsCollections;
		declare $limit as Uint64;
		declare $stalling_time as Timestamp;
		declare $typeWhitelist as List<Utf8>;
		declare $typeBlacklist as List<Utf8>;
		declare $local_zone_id as Utf8;

		select * from %v
		where
			(modified_at < $stalling_time) and
			(ListLength($typeWhitelist) == 0 or task_type in $typeWhitelist) and
			(task_type not in $typeBlacklist) and
			(zone_id is null or Len(zone_id) == 0 or zone_id == $local_zone_id)
		limit $limit
	`, s.tablesPath, tableName), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(limit)),
		ydb_table.ValueParam("$stalling_time", persistence.TimestampValue(stallingTime)),
		ydb_table.ValueParam("$typeWhitelist", strListValue(s.taskTypeWhitelist)),
		ydb_table.ValueParam("$typeBlacklist", strListValue(s.taskTypeBlacklist)),
		ydb_table.ValueParam("$local_zone_id", ydb_types.UTF8Value(s.ZoneID)),
	))
	if err != nil {
		return nil, err
	}
	defer res.Close()

	return scanTaskInfos(ctx, res)
}

func (s *storageYDB) lockTaskToExecute(
	ctx context.Context,
	session *persistence.Session,
	taskInfo TaskInfo,
	at time.Time,
	acceptableStatus func(TaskStatus) bool,
	newStatus TaskStatus,
	hostname string,
	runner string,
) (TaskState, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return TaskState{}, err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from tasks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(taskInfo.ID)),
	))
	if err != nil {
		return TaskState{}, err
	}
	defer res.Close()

	states, err := s.scanTaskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return TaskState{}, commitErr
		}

		return TaskState{}, fmt.Errorf("failed to scan []TaskState: %w", err)
	}

	if len(states) == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return TaskState{}, err
		}

		return TaskState{}, &errors.NonRetriableError{
			Err: &errors.NotFoundError{TaskID: taskInfo.ID},
		}
	}

	state := states[0]
	if state.GenerationID != taskInfo.GenerationID {
		return TaskState{}, &errors.WrongGenerationError{}
	}

	lastState := state.DeepCopy()

	if !acceptableStatus(lastState.Status) {
		err = tx.Commit(ctx)
		if err != nil {
			return TaskState{}, err
		}

		return TaskState{}, &errors.NonRetriableError{
			Err: fmt.Errorf("invalid status %v", lastState.Status),
		}
	}

	state.Status = newStatus
	state.GenerationID++
	state.ModifiedAt = at
	state.LastHost = hostname
	state.LastRunner = runner
	state.ChangedStateAt = lastState.ChangedStateAt
	if lastState.Status != state.Status {
		state.ChangedStateAt = at
	}

	transition := stateTransition{
		lastState: &lastState,
		newState:  state,
	}
	err = s.updateTaskStates(ctx, tx, []stateTransition{transition})
	if err != nil {
		return TaskState{}, fmt.Errorf("failed to lock task for executing with id=%v: %w", state.ID, err)
	}

	err = tx.Commit(ctx)
	if err != nil {
		return TaskState{}, err
	}

	return state, nil
}

func (s *storageYDB) prepareDependenciesToClear(
	ctx context.Context,
	tx *persistence.Transaction,
	state *TaskState,
) ([]stateTransition, error) {

	transitions := make([]stateTransition, 0)

	dependencyIDs := state.Dependencies.List()
	for _, id := range dependencyIDs {
		res, err := tx.Execute(ctx, fmt.Sprintf(`
			--!syntax_v1
			pragma TablePathPrefix = "%v";
			declare $id as Utf8;

			select *
			from tasks
			where id = $id
		`, s.tablesPath),
			ydb_table.NewQueryParameters(
				ydb_table.ValueParam("$id", ydb_types.UTF8Value(id)),
			))
		if err != nil {
			return []stateTransition{}, err
		}
		defer res.Close()

		dependencies, err := s.scanTaskStates(ctx, res)
		if err != nil {
			commitErr := tx.Commit(ctx)
			if commitErr != nil {
				return []stateTransition{}, commitErr
			}

			return []stateTransition{}, err
		}

		state.Dependencies.Remove(id)

		if len(dependencies) != 0 {
			newState := dependencies[0].DeepCopy()
			newState.dependants.Remove(state.ID)

			transitions = append(transitions, stateTransition{
				lastState: &dependencies[0],
				newState:  newState,
			})
		}
	}

	return transitions, nil
}

func (s *storageYDB) prepareDependantsToWakeup(
	ctx context.Context,
	tx *persistence.Transaction,
	state *TaskState,
) ([]stateTransition, error) {

	ids := make([]ydb_types.Value, 0)
	for _, id := range state.dependants.List() {
		ids = append(ids, ydb_types.UTF8Value(id))
	}

	if len(ids) == 0 {
		return []stateTransition{}, nil
	}

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $ids as List<Utf8>;

		select *
		from tasks
		where id in $ids
	`, s.tablesPath),
		ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$ids", ydb_types.ListValue(ids...)),
		))
	if err != nil {
		return []stateTransition{}, err
	}
	defer res.Close()

	dependants, err := s.scanTaskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return []stateTransition{}, commitErr
		}

		return []stateTransition{}, err
	}

	transitions := make([]stateTransition, 0)

	for i := 0; i < len(dependants); i++ {
		newState := dependants[i].DeepCopy()
		newState.Dependencies.Remove(state.ID)

		if len(newState.Dependencies.Vals()) == 0 {
			// Return from "sleeping" state because dependencies are resolved.
			switch newState.Status {
			case TaskStatusWaitingToRun:
				newState.Status = TaskStatusReadyToRun
				newState.GenerationID++

			case TaskStatusWaitingToCancel:
				newState.Status = TaskStatusReadyToCancel
				newState.GenerationID++
			}
		}

		transitions = append(transitions, stateTransition{
			lastState: &dependants[i],
			newState:  newState,
		})

		state.dependants.Remove(dependants[i].ID)
	}

	return transitions, nil
}

func (s *storageYDB) markForCancellation(
	ctx context.Context,
	session *persistence.Session,
	taskID string,
	at time.Time,
) (bool, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return false, err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from tasks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(taskID)),
	))
	if err != nil {
		return false, err
	}
	defer res.Close()

	states, err := s.scanTaskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return false, commitErr
		}

		return false, fmt.Errorf("failed to scan []TaskState: %w", err)
	}

	if len(states) == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return false, err
		}

		return false, &errors.NonRetriableError{
			Err: &errors.NotFoundError{TaskID: taskID},
		}
	}
	state := states[0]

	if IsCancelling(state.Status) {
		err = tx.Commit(ctx)
		if err != nil {
			return false, err
		}

		return true, nil
	}

	if state.Status == TaskStatusFinished {
		return false, tx.Commit(ctx)
	}

	lastState := state.DeepCopy()

	state.Status = TaskStatusReadyToCancel
	state.GenerationID++
	state.ModifiedAt = at
	state.ChangedStateAt = at
	state.ErrorMessage = "Cancelled by client"
	state.ErrorCode = grpc_codes.Canceled

	transitions, err := s.prepareDependenciesToClear(ctx, tx, &state)
	if err != nil {
		return false, err
	}

	wakeupTransitions, err := s.prepareDependantsToWakeup(ctx, tx, &state)
	if err != nil {
		return false, err
	}

	transitions = append(transitions, wakeupTransitions...)
	transitions = append(transitions, stateTransition{
		lastState: &lastState,
		newState:  state,
	})
	err = s.updateTaskStates(ctx, tx, transitions)
	if err != nil {
		return false, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return false, err
	}

	return true, nil
}

func (s *storageYDB) decrementRegularTasksInflight(
	ctx context.Context,
	tx *persistence.Transaction,
	taskType string,
) error {

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $task_type as Utf8;

		select *
		from schedules
		where task_type = $task_type
	`, s.tablesPath),
		ydb_table.NewQueryParameters(
			ydb_table.ValueParam("$task_type", ydb_types.UTF8Value(taskType)),
		))
	if err != nil {
		return err
	}
	defer res.Close()

	found := false
	sch := schedule{}

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("task_type", &sch.taskType),
				ydb_named.OptionalWithDefault("scheduled_at", &sch.scheduledAt),
				ydb_named.OptionalWithDefault("tasks_inflight", &sch.tasksInflight),
			)
			if err != nil {
				return &errors.NonRetriableError{
					Err: fmt.Errorf("decrementRegularTasksInflight: failed to parse row: %w", err),
				}
			}
			found = true
		}
	}

	if err = res.Err(); err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return commitErr
		}
		return err
	}

	if !found {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf("schedule %v is not found", taskType),
		}
	}

	if sch.tasksInflight == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return err
		}

		return &errors.NonRetriableError{
			Err: fmt.Errorf("schedule %v should have tasksInflight greater than zero", taskType),
		}
	}

	sch.tasksInflight--

	_, err = tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $task_type as Utf8;
		declare $scheduled_at as Timestamp;
		declare $tasks_inflight as Uint64;

		upsert into schedules (task_type, scheduled_at, tasks_inflight)
		values ($task_type, $scheduled_at, $tasks_inflight)
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$task_type", ydb_types.UTF8Value(sch.taskType)),
		ydb_table.ValueParam("$scheduled_at", persistence.TimestampValue(sch.scheduledAt)),
		ydb_table.ValueParam("$tasks_inflight", ydb_types.Uint64Value(sch.tasksInflight)),
	))
	return err
}

func (s *storageYDB) updateTask(
	ctx context.Context,
	session *persistence.Session,
	state TaskState,
) (TaskState, error) {

	tx, err := session.BeginRWTransaction(ctx)
	if err != nil {
		return TaskState{}, err
	}
	defer tx.Rollback(ctx)

	res, err := tx.Execute(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $id as Utf8;

		select *
		from tasks
		where id = $id
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$id", ydb_types.UTF8Value(state.ID)),
	))
	if err != nil {
		return TaskState{}, err
	}
	defer res.Close()

	states, err := s.scanTaskStates(ctx, res)
	if err != nil {
		commitErr := tx.Commit(ctx)
		if commitErr != nil {
			return TaskState{}, commitErr
		}

		return TaskState{}, fmt.Errorf("failed to scan []TaskState: %w", err)
	}

	if len(states) == 0 {
		err = tx.Commit(ctx)
		if err != nil {
			return TaskState{}, err
		}

		return TaskState{}, &errors.NonRetriableError{
			Err: &errors.NotFoundError{TaskID: state.ID},
		}
	}

	lastState := states[0]

	if lastState.GenerationID != state.GenerationID {
		return TaskState{}, &errors.WrongGenerationError{}
	}

	state.ChangedStateAt = lastState.ChangedStateAt
	state.EndedAt = lastState.EndedAt

	if lastState.Status != state.Status {
		state.ChangedStateAt = state.ModifiedAt

		if HasEnded(state.Status) {
			state.EndedAt = state.ModifiedAt
		}

		state.GenerationID++
	}
	// Always inherit dependants from previous state.
	state.dependants = lastState.dependants.DeepCopy()

	transitions, err := s.prepareUnfinishedDependencies(ctx, tx, &state)
	if err != nil {
		return TaskState{}, err
	}

	shouldInterruptTaskExecution := false

	if len(transitions) != 0 {
		// Return to "sleeping" state until dependencies are resolved.
		switch state.Status {
		case TaskStatusRunning:
			state.Status = TaskStatusWaitingToRun
			state.GenerationID++
			shouldInterruptTaskExecution = true

		case TaskStatusCancelling:
			state.Status = TaskStatusWaitingToCancel
			state.GenerationID++
			shouldInterruptTaskExecution = true
		}
	}

	if HasResult(state.Status) {
		dependants, err := s.prepareDependantsToWakeup(ctx, tx, &state)
		if err != nil {
			return TaskState{}, err
		}

		transitions = append(transitions, dependants...)
	}

	if HasEnded(state.Status) && state.Regular {
		err := s.decrementRegularTasksInflight(ctx, tx, state.TaskType)
		if err != nil {
			return TaskState{}, err
		}
	}

	transitions = append(transitions, stateTransition{
		lastState: &lastState,
		newState:  state,
	})
	err = s.updateTaskStates(ctx, tx, transitions)
	if err != nil {
		return TaskState{}, err
	}

	err = tx.Commit(ctx)
	if err != nil {
		return TaskState{}, err
	}

	if shouldInterruptTaskExecution {
		// State has been updated but execution is not possible within current
		// generation.
		return TaskState{}, &errors.InterruptExecutionError{}
	}

	// Returned state should have old generation id, this is needed to protect
	// task from further updating within current execution.
	state.GenerationID = lastState.GenerationID

	s.metrics.OnTaskUpdated(state)
	return state, nil
}

func (s *storageYDB) clearEndedTasks(
	ctx context.Context,
	session *persistence.Session,
	endedBefore time.Time,
	limit int,
) error {

	_, res, err := session.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $ended_before as Timestamp;
		declare $limit as Uint64;

		select *
		from ended
		where ended_at < $ended_before
		limit $limit
	`, s.tablesPath), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$ended_before", persistence.TimestampValue(endedBefore)),
		ydb_table.ValueParam("$limit", ydb_types.Uint64Value(uint64(limit))),
	))
	if err != nil {
		return err
	}
	defer res.Close()

	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var (
				endedAt        time.Time
				taskID         string
				idempotencyKey string
				accountID      string
			)
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("ended_at", &endedAt),
				ydb_named.OptionalWithDefault("id", &taskID),
				ydb_named.OptionalWithDefault("idempotency_key", &idempotencyKey),
				ydb_named.OptionalWithDefault("account_id", &accountID),
			)
			if err != nil {
				return &errors.NonRetriableError{
					Err: fmt.Errorf("clearEndedTasks: failed to parse row: %w", err),
				}
			}

			execute := func(deleteFromTaskIds string) error {
				_, _, err = session.ExecuteRW(ctx, fmt.Sprintf(`
					--!syntax_v1
					pragma TablePathPrefix = "%v";
					declare $ended_at as Timestamp;
					declare $task_id as Utf8;
					declare $idempotency_key as Utf8;
					declare $account_id as Utf8;
					declare $finished as Int64;
					declare $cancelled as Int64;

					delete from tasks
					where id = $task_id and (status = $finished or status = $cancelled);

					%v

					delete from ended
					where ended_at = $ended_at and id = $task_id
				`, s.tablesPath, deleteFromTaskIds), ydb_table.NewQueryParameters(
					ydb_table.ValueParam("$ended_at", persistence.TimestampValue(endedAt)),
					ydb_table.ValueParam("$task_id", ydb_types.UTF8Value(taskID)),
					ydb_table.ValueParam("$idempotency_key", ydb_types.UTF8Value(idempotencyKey)),
					ydb_table.ValueParam("$account_id", ydb_types.UTF8Value(accountID)),
					ydb_table.ValueParam("$finished", ydb_types.Int64Value(int64(TaskStatusFinished))),
					ydb_table.ValueParam("$cancelled", ydb_types.Int64Value(int64(TaskStatusCancelled))),
				))
				return err
			}

			if len(idempotencyKey) == 0 {
				err = execute("")
			} else {
				err = execute(`
					delete from task_ids
					where idempotency_key = $idempotency_key and account_id = $account_id;
				`)
			}
			if err != nil {
				return err
			}
		}
	}

	return res.Err()
}
