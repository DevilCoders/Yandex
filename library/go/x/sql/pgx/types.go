package pgx

// Unlike Go Postgres can distinguish various string types.
// These are helper types for Array function.
// Example:
//    states := []string{"NEW", "ACTIVE", "PROCESSED"}
//    arr, _ := pgxutil.Array(EnumArray(states))
type TextArray []string
type VarcharArray []string
type EnumArray []string
