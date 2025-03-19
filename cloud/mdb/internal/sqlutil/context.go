package sqlutil

type ctxKey string

func (c ctxKey) String() string {
	return "mdb/sqlutil context key " + string(c)
}
