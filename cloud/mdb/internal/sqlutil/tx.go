package sqlutil

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
)

// InTx executes provided function inside transaction and automatically commits or rollbacks
func InTx(ctx context.Context, node Node, txFunc func(TxBinding) error) error {
	binding, err := BeginOnNode(ctx, node, nil)
	if err != nil {
		return err
	}
	defer func() {
		_ = binding.Rollback(ctx)
	}()

	if err = txFunc(binding); err == nil {
		return binding.Commit(ctx)
	}

	return err
}

type QueryCallback func(stmt Stmt, args map[string]interface{}, parser RowParser) error

// InTxDoQueries executes handler in transaction. That handler takes one argument with which it should execute queries
func InTxDoQueries(ctx context.Context, node Node, l log.Logger, handler func(QueryCallback) error) error {
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	return InTx(
		ctx,
		node,
		func(binding TxBinding) error {
			return handler(func(stmt Stmt, args map[string]interface{}, parser RowParser) error {
				if _, err := QueryTxBinding(
					ctx,
					binding,
					stmt,
					args,
					parser,
					l,
				); err != nil {
					return fmt.Errorf("%s query failed: %w", stmt.Name, err)
				}

				return nil
			})
		},
	)
}
