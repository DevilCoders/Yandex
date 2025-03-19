package tags

import (
	"github.com/opentracing/opentracing-go/ext"

	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
)

const (
	OperationDBQuery        = "DB Query"
	OperationDBQueryExecute = "DB Query Execute"
	OperationDBQueryResult  = "DB Query Result"
	OperationDBTxBegin      = "DB TxBegin"
	OperationDBTxCommit     = "DB TxCommit"
	OperationDBTxRollback   = "DB TxRollback"
)

var (
	DBInstance         = tags.StringTagName(ext.DBInstance)
	DBType             = tags.StringTagName(ext.DBType)
	DBUser             = tags.StringTagName(ext.DBUser)
	DBStatementName    = tags.StringTagName("db.statement.name")
	DBStatement        = tags.StringTagName(ext.DBStatement)
	DBQueryResultCount = tags.Int64TagName("db.query.result.count")

	RedisCommand       = tags.StringTagName("redis.cmd")
	RedisCommands      = tags.StringsTagName("redis.cmds")
	RedisCommandsCount = tags.IntTagName("redis.num_cmd")
	RedisConnInfo      = tags.StringTagName("redis.conn_info")
)
