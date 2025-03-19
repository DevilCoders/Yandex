package operations

// TODO: move all operation models code into separate package

import (
	"encoding/json"
	"fmt"
	"reflect"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

// Database is database type
type Database int

const (
	DatabaseUnknown = iota
	DatabaseClickhouse
	DatabaseElasticsearch
	DatabaseKafka
	DatabaseMetastore
	DatabaseMongodb
	DatabasePostgresql
	DatabaseSqlserver
	DatabaseGreenplum
	DatabaseOpenSearch
)

var (
	mapStringToDatabase = map[string]Database{
		"clickhouse":    DatabaseClickhouse,
		"elasticsearch": DatabaseElasticsearch,
		"kafka":         DatabaseKafka,
		"metastore":     DatabaseMetastore,
		"mongodb":       DatabaseMongodb,
		"postgresql":    DatabasePostgresql,
		"sqlserver":     DatabaseSqlserver,
		"greenplum":     DatabaseGreenplum,
		"opensearch":    DatabaseOpenSearch,
	}
)

// Type is operation type
type Type string

type Descriptor struct {
	Description  string
	MetadataType reflect.Type
}

var _ valid.Validator = &Descriptor{}

func (desc Descriptor) Validate(_ *valid.ValidationCtx) (bool, error) {
	if desc.Description == "" {
		return true, xerrors.Errorf("operation descriptor missing description: %+v", desc)
	}

	kind := reflect.TypeOf(desc.MetadataType).Elem().Kind()
	if kind != reflect.Struct {
		return true, xerrors.Errorf("metadata type must be a struct value but is %q instead: %+v", kind, desc)
	}

	return true, nil
}

var opTypes = map[Type]Descriptor{}

func Register(ot Type, description string, md Metadata) {
	if _, ok := opTypes[ot]; ok {
		panic(fmt.Sprintf("operation type %q is already registered", ot))
	}

	descriptor := Descriptor{Description: description, MetadataType: reflect.TypeOf(md)}

	if _, err := descriptor.Validate(nil); err != nil {
		panic(fmt.Sprintf("failed to validate operation %q descriptor: %s", ot, err))
	}

	opTypes[ot] = descriptor
}

func GetDescriptor(ot Type) (Descriptor, error) {
	opdesc, ok := opTypes[ot]
	if !ok {
		return Descriptor{}, xerrors.Errorf("unknown operation type: %s", ot)
	}

	return opdesc, nil
}

func ParseType(str string) (Type, error) {
	ot := Type(str)
	if _, ok := opTypes[ot]; !ok {
		return "", xerrors.Errorf("unknown operation type: %s", str)
	}

	return ot, nil
}

func (tp Type) Database() (Database, error) {
	parts := strings.Split(string(tp), "_")
	if len(parts) == 0 {
		return DatabaseUnknown, xerrors.Errorf("unknown database: %s", string(tp))
	}
	dbname := parts[0]
	db, ok := mapStringToDatabase[dbname]
	if !ok {
		return DatabaseUnknown, xerrors.Errorf("unknown database: %s", string(tp))
	}
	return db, nil
}

// Operation in cloud (task in worker)
type Operation struct {
	OperationID         string
	TargetID            string
	ClusterID           string
	ClusterType         clusters.Type
	Environment         string
	Type                Type
	CreatedBy           string
	CreatedAt           time.Time
	StartedAt           time.Time
	ModifiedAt          time.Time
	Status              Status
	MetaData            interface{}
	Args                json.RawMessage
	Hidden              bool
	RequiredOperationID string
	Errors              []OperationError
}

type Status string

func (os Status) IsTerminal() bool {
	return os == StatusDone || os == StatusFailed
}

const (
	StatusPending Status = "PENDING"
	StatusRunning Status = "RUNNING"
	StatusDone    Status = "DONE"
	StatusFailed  Status = "FAILED"
)

var opStatusMap = map[Status]struct{}{
	StatusPending: {},
	StatusRunning: {},
	StatusDone:    {},
	StatusFailed:  {},
}

func ParseStatus(str string) (Status, error) {
	os := Status(str)
	if _, ok := opStatusMap[os]; !ok {
		return "", xerrors.Errorf("unknown operation status: %s", str)
	}

	return os, nil
}

type OperationError struct {
	Exposable bool             `json:"exposable"`
	Code      int              `json:"code"`
	Message   string           `json:"message"`
	Details   []OperationError `json:"details"`
}

type OperationsPageToken struct {
	OperationID          string
	OperationIDCreatedAt time.Time
	More                 bool `json:"-"`
}

func (opt OperationsPageToken) HasMore() bool {
	return opt.More
}

func NewOperationsPageToken(actualSize int64, expectedPageSize int64, lastOpID string, lastOpIDCreatedAt time.Time) OperationsPageToken {
	var nextOpPageToken OperationsPageToken
	if actualSize == expectedPageSize {
		nextOpPageToken = OperationsPageToken{
			OperationID:          lastOpID,
			OperationIDCreatedAt: lastOpIDCreatedAt,
			More:                 true,
		}
	}

	return nextOpPageToken
}
