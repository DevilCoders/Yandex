package mysqlutil

// SlaveStatus represents SHOW SLAVE STATUS results
type SlaveStatus struct {
	MasterHost       string `db:"Master_Host"`
	MasterPort       int    `db:"Master_Port"`
	MasterLogFile    string `db:"Master_Log_File"`
	SlaveIORunning   string `db:"Slave_IO_Running"`
	SlaveSQLRunning  string `db:"Slave_SQL_Running"`
	RetrievedGtidSet string `db:"Retrieved_Gtid_Set"`
	ExecutedGtidSet  string `db:"Executed_Gtid_Set"`
	LastIOErrno      int    `db:"Last_IO_Errno"`
	LastIOError      string `db:"Last_IO_Error"`
	LastSQLErrno     int    `db:"Last_SQL_Errno"`
	LastSQLError     string `db:"Last_SQL_Error"`
}

// GTIDExecuted contains SHOW MASTER STATUS response
type GTIDExecuted struct {
	ExecutedGtidSet string `db:"Executed_Gtid_Set"`
}

// Binlog represents SHOW BINARY LOGS result item
type Binlog struct {
	Name string `db:"Log_name"`
	Size int64  `db:"File_size"`
}
