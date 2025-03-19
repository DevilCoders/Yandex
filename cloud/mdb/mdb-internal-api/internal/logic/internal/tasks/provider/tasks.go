package provider

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	logictasks "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Tasks struct {
	l           log.Logger
	cfg         logic.Config
	search      search.Docs
	metaDB      metadb.Backend
	idGenerator generator.IDGenerator
}

var _ logictasks.Tasks = &Tasks{}

func NewTasks(metaDB metadb.Backend, search search.Docs, gen generator.IDGenerator, cfg logic.Config, l log.Logger) *Tasks {
	return &Tasks{metaDB: metaDB, search: search, idGenerator: gen, cfg: cfg, l: l}
}

func (t *Tasks) CreateTask(ctx context.Context, session sessions.Session, args tasks.CreateTaskArgs) (operations.Operation, error) {
	if !args.SkipIdempotence {
		idemp, ok := idempotence.IncomingFromContext(ctx)
		if ok {
			args.Idempotence = &idemp
		}
	}
	if err := args.Validate(); err != nil {
		return operations.Operation{}, xerrors.Errorf("failed to validate create task arguments: %w", err)
	}

	if args.TaskType.IsClusterStop() && !t.cfg.ClusterStopSupported {
		return operations.Operation{}, semerr.NotImplemented("cluster stop is not supported for the current installation")
	}

	if !args.TaskType.IsMetaDelete() && !args.SkipConflictingTask {
		runningID, err := t.metaDB.RunningTaskID(ctx, args.ClusterID)
		if err != nil && !xerrors.Is(err, sqlerrors.ErrNotFound) {
			return operations.Operation{}, xerrors.Errorf("failed to check for conflicting task: %w", err)
		} else if err == nil {
			return operations.Operation{}, semerr.FailedPreconditionf("conflicting operation %q detected", runningID)
		}
	}

	// Don't check failed tasks for cluster deletion
	if !args.TaskType.IsClusterDelete() && !args.TaskType.IsMetaDelete() {
		failedID, err := t.metaDB.FailedTaskID(ctx, args.ClusterID)
		if err != nil && !xerrors.Is(err, sqlerrors.ErrNotFound) {
			return operations.Operation{}, xerrors.Errorf("failed to check for failed task: %w", err)
		} else if err == nil {
			return operations.Operation{}, semerr.FailedPreconditionf("cluster state is inconsistent due to operation %q failure", failedID)
		}
	}

	// Generate task id if needed
	if args.TaskID == "" {
		id, err := t.idGenerator.Generate()
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("task id not generated: %w", err)
		}

		args.TaskID = id
	}

	if args.TaskArgs == nil {
		args.TaskArgs = map[string]interface{}{}
	}
	args.TaskArgs["feature_flags"] = session.FeatureFlags.List()

	tracingCarrier := opentracing.TextMapCarrier{}
	if span := opentracing.SpanFromContext(ctx); span != nil {
		if err := opentracing.GlobalTracer().Inject(
			span.Context(),
			opentracing.TextMap,
			tracingCarrier,
		); err != nil {
			ctxlog.Error(ctx, t.l, "trace span textmap injection failed", log.Error(err))
			sentry.GlobalClient().CaptureError(ctx, err, nil)
		}
	}

	op, err := t.metaDB.CreateTask(ctx, args, tracingCarrier)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("failed to create task in metadb: %w", err)
	}

	// TODO
	/*if !op.Hidden {
		common.mdb.CreateEvent
	}*/

	return op, nil
}

func (t *Tasks) CreateFinishedTask(ctx context.Context, session sessions.Session, cid string, revision int64, opType operations.Type, metadata interface{}, atCurrentRevision bool) (operations.Operation, error) {
	var op operations.Operation
	var err error
	args := tasks.CreateFinishedTaskArgs{
		ClusterID:     cid,
		FolderID:      session.FolderCoords.FolderID,
		OperationType: opType,
		Auth:          session.Subject,
		Revision:      revision,
		Metadata:      metadata,
	}

	idemp, ok := idempotence.IncomingFromContext(ctx)
	if ok {
		args.Idempotence = &idemp
	}

	// Generate task id if needed
	id, err := t.idGenerator.Generate()
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("finished task for operation %q: task id not generated: %w", opType, err)
	}
	args.TaskID = id

	if err = args.Validate(); err != nil {
		return operations.Operation{}, xerrors.Errorf("finished task for operation %q not created: %w", opType, err)
	}

	if atCurrentRevision {
		op, err = t.metaDB.CreateFinishedTaskAtCurrentRev(ctx, args)
	} else {
		op, err = t.metaDB.CreateFinishedTask(ctx, args)
	}
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("finished task for operation %q not created: %w", opType, err)
	}

	return op, nil
}
