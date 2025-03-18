package yt

type (
	TableReader interface {
		Scan(value interface{}) error
		Next() bool
		Err() error
		Close() error
	}

	Client interface {
		SelectRows() (TableReader, error)
		LookupRows() (TableReader, error)
		ReadTable() (TableReader, error)
	}
)
