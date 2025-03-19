package sqlutil

import (
	"context"
	"database/sql"

	"github.com/jmoiron/sqlx"
	"github.com/opentracing/opentracing-go"
	hasql "golang.yandex/hasql/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type (
	ClusterOption     = hasql.ClusterOption
	Node              = hasql.Node
	NodeStateCriteria = hasql.NodeStateCriteria
	NodeChecker       = hasql.NodeChecker
	NodePicker        = hasql.NodePicker
	Tracer            = hasql.Tracer
)

var (
	NewNode = hasql.NewNode

	Alive         = hasql.Alive
	Primary       = hasql.Primary
	Standby       = hasql.Standby
	PreferPrimary = hasql.PreferPrimary
	PreferStandby = hasql.PreferStandby

	WithUpdateInterval = hasql.WithUpdateInterval
	WithUpdateTimeout  = hasql.WithUpdateTimeout
	WithNodePicker     = hasql.WithNodePicker
	WithTracer         = hasql.WithTracer

	PickNodeRandom     = hasql.PickNodeRandom
	PickNodeRoundRobin = hasql.PickNodeRoundRobin
	PickNodeClosest    = hasql.PickNodeClosest
)

type Cluster struct {
	*hasql.Cluster
}

func NewCluster(nodes []Node, checkNode NodeChecker, opts ...ClusterOption) (*Cluster, error) {
	cl, err := hasql.NewCluster(nodes, checkNode, opts...)
	if err != nil {
		return nil, err
	}

	return &Cluster{Cluster: cl}, nil
}

// NodeChooser is a function that somehow chooses cluster node
type NodeChooser func(ctx context.Context) Node

// AliveChooser implements NodeChooser for alive node
func (cl *Cluster) AliveChooser() NodeChooser {
	return func(context.Context) Node {
		return cl.Alive()
	}
}

// PrimaryChooser implements NodeChooser for primary node
func (cl *Cluster) PrimaryChooser() NodeChooser {
	return func(context.Context) Node {
		return cl.Primary()
	}
}

// StandbyChooser implements NodeChooser for standby node
func (cl *Cluster) StandbyChooser() NodeChooser {
	return func(context.Context) Node {
		return cl.Standby()
	}
}

// MasterPreferredChooser implements NodeChooser for preferably master node
func (cl *Cluster) MasterPreferredChooser() NodeChooser {
	return func(context.Context) Node {
		return cl.PrimaryPreferred()
	}
}

// ReplicaPreferredChooser implements NodeChooser for preferably replica node
func (cl *Cluster) ReplicaPreferredChooser() NodeChooser {
	return func(context.Context) Node {
		return cl.StandbyPreferred()
	}
}

// TxBinder provides interface for starting and binding transaction on specified node to context
type TxBinder interface {
	// Begin transaction on node of specified type and bind it to context
	Begin(ctx context.Context, ns hasql.NodeStateCriteria) (context.Context, error)
	// Commit transaction bound to context
	Commit(ctx context.Context) error
	// Rollback transaction bound to context
	Rollback(ctx context.Context) error
}

// TxBinding holds transaction and node it is initiated on.
type TxBinding struct {
	node Node
	tx   *sqlx.Tx
}

func Begin(ctx context.Context, c *Cluster, ns hasql.NodeStateCriteria, opts *sql.TxOptions) (TxBinding, error) {
	node := c.Node(ns)
	if node == nil {
		return TxBinding{}, semerr.Unavailable("unavailable")
	}

	return BeginOnNode(ctx, node, opts)
}

func BeginOnNode(ctx context.Context, node Node, opts *sql.TxOptions) (TxBinding, error) {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBTxBegin,
		tags.DBType.Tag("sql"),
		tags.DBInstance.Tag(node.Addr()),
	)
	defer span.Finish()

	tx, err := node.DBx().BeginTxx(ctx, opts)
	if err != nil {
		tracing.SetErrorOnSpan(span, err)
		return TxBinding{}, semerr.WrapWithUnavailable(err, "unavailable")
	}

	return TxBinding{node: node, tx: tx}, nil
}

func (b TxBinding) Commit(ctx context.Context) error {
	if b.tx == nil {
		return xerrors.New("no transaction in context")
	}

	span, _ := opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBTxCommit,
		tags.DBType.Tag("sql"),
		tags.DBInstance.Tag(b.node.Addr()),
	)
	defer span.Finish()

	if err := b.tx.Commit(); err != nil {
		tracing.SetErrorOnSpan(span, err)
		return err
	}

	return nil
}

func (b TxBinding) Rollback(ctx context.Context) error {
	if b.tx == nil {
		return xerrors.New("no transaction in context")
	}

	span, _ := opentracing.StartSpanFromContext(
		ctx,
		tags.OperationDBTxRollback,
		tags.DBType.Tag("sql"),
		tags.DBInstance.Tag(b.node.Addr()),
	)
	defer span.Finish()

	// We ignore all rollback errors on purpose - tx is rolled back anyway
	// and we do not care how it happened exactly
	_ = b.tx.Rollback()
	return nil
}

// Tx returns bound transaction
// DEPRECATED: use Query* functions, do not use Tx directly
func (b TxBinding) Tx() *sqlx.Tx {
	return b.tx
}

var (
	ctxKeyTxBinding = ctxKey("tx binding")
)

// WithTxBinding adds tx binding to context
func WithTxBinding(ctx context.Context, b TxBinding) context.Context {
	return context.WithValue(ctx, ctxKeyTxBinding, b)
}

// BindingFrom retrieves tx binding from context
func TxBindingFrom(ctx context.Context) (TxBinding, bool) {
	b := ctx.Value(ctxKeyTxBinding)
	if b == nil {
		return TxBinding{}, false
	}

	binding, ok := b.(TxBinding)
	return binding, ok
}
