package misc

const (
	// TableOpNone is a shortcut for no options
	TableOpNone = 0

	// TableOpCreate asks for table creation
	TableOpCreate = 1 << iota

	// TableOpDrop asks for table dropping
	TableOpDrop

	// TableOpDBOnly asks to ignore non-db-related errors
	// non create proxy for listen docker sockets
	// can use facade for database operation only (for example can't import image from url)
	TableOpDBOnly
)
