package kikimr

import (
	"strings"
	"time"
)

// Single KiKiMR database settings
type DatabaseConfig struct {
	DBHost string // host to use on new driver
	DBName string // name of database, starts with "/". Unused in old driver
	Root   string // database_name/path_to_tables

	TLS bool

	MaxConns           int
	MaxSelectRows      int
	OperationTimeoutMs int // Timeout for YDB request (default 1 hour).

	EnableLogging bool

	UseBigTablesProfile bool // shell we use ColumnProfiles(public ydb) (off for ci)
	MaxSessionCount     int

	LockPollIntervalSec int
	TimeSkewSec         int
	UpdateGraceTimeSec  int

	TraceDriver      bool
	TraceClient      bool
	TraceSessionPool bool
}

// Database name to settings
type Config map[string]DatabaseConfig

func (c *DatabaseConfig) GetTableName(tableName string) string {
	return "[" + strings.TrimRight(c.Root, "/") + "/" + tableName + "]"
}

func (c *DatabaseConfig) GetMaxSelectRows() int {
	if c.MaxSelectRows <= 0 {
		return 1000
	}
	return c.MaxSelectRows
}

func (c *DatabaseConfig) Validate() error {
	if c.OperationTimeoutMs == 0 {
		c.OperationTimeoutMs = int(time.Hour / time.Millisecond)
	}
	return nil
}
