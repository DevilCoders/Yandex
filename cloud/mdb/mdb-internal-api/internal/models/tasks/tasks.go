package tasks

import (
	"reflect"
	"strings"

	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Type used for worker tasks
type Type string

func (typ Type) IsClusterDelete() bool {
	return strings.HasSuffix(string(typ), "cluster_delete")
}

func (typ Type) IsClusterStop() bool {
	return strings.HasSuffix(string(typ), "cluster_stop")
}

func (typ Type) IsMetaDelete() bool {
	return strings.HasSuffix(string(typ), "cluster_delete_metadata") || strings.HasSuffix(string(typ), "cluster_purge")
}

// CreateTaskArgs contains all necessary arguments for creation of worker tasks
type CreateTaskArgs struct {
	TaskID        string
	ClusterID     string
	FolderID      int64
	OperationType operations.Type
	TaskType      Type
	TaskArgs      map[string]interface{}
	Metadata      operations.Metadata
	Auth          as.Subject
	Hidden        bool
	Timeout       optional.Duration
	Idempotence   *idempotence.Incoming
	// if a single request handler creates multiple worker tasks when processing
	// a grpc-request that comes with idempotence-id the second task being created
	// will fail on pk_idempotence constraint as the same idempotence id would be used
	// that's why need to skip idempotence logic for some tasks (metadata deletion, purge).
	// https://st.yandex-team.ru/MDB-9336
	SkipIdempotence     bool
	SkipConflictingTask bool
	DelayBy             optional.Duration
	RequiredOperationID optional.String
	Revision            int64
}

func (cta CreateTaskArgs) Validate() error {
	if cta.ClusterID == "" {
		return xerrors.New("empty cluster id")
	}

	if cta.FolderID == 0 {
		return xerrors.New("zero folder id")
	}

	// TODO: validate earlier when we check auth?
	if cta.Auth.IsEmpty() {
		return xerrors.New("empty auth")
	}

	if cta.TaskType == "" {
		return xerrors.New("empty task type")
	}

	if cta.OperationType == "" {
		return xerrors.New("empty operation type")
	}

	desc, err := operations.GetDescriptor(cta.OperationType)
	if err != nil {
		return xerrors.Errorf("unknown operation type %q: %w", cta.OperationType, err)
	}

	// Validate metadata only if it is provided
	if cta.Metadata != nil {
		mdType := reflect.TypeOf(cta.Metadata)
		if mdType != desc.MetadataType {
			// TODO: non-fatal - report to sentry and continue
			return xerrors.Errorf("expected operation %q metadata type %q but got %q instead", cta.OperationType, desc.MetadataType, mdType)
		}
	}

	if cta.Revision == 0 {
		return xerrors.New("zero revision")
	}

	return nil
}

// CreateFinishedTaskArgs contains all necessary arguments for creating finished worker task
type CreateFinishedTaskArgs struct {
	TaskID        string
	ClusterID     string
	FolderID      int64
	OperationType operations.Type
	Metadata      interface{}
	Auth          as.Subject
	Idempotence   *idempotence.Incoming
	Revision      int64
}

func (args CreateFinishedTaskArgs) Validate() error {
	if args.ClusterID == "" {
		return xerrors.New("empty cluster id")
	}

	if args.FolderID == 0 {
		return xerrors.New("zero folder id")
	}

	if args.Auth.IsEmpty() {
		return xerrors.New("empty auth")
	}

	if args.OperationType == "" {
		return xerrors.New("empty operation type")
	}

	if args.Revision == 0 {
		return xerrors.New("zero revision")
	}

	return nil
}
