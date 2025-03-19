package ydblock

// Query registry shamelessly copy-pasted from
// https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/healthcheck/yql/client.go#37

type queryID uint

// QueryInfo have prerendered information about YQL query.
type QueryInfo struct {
	rawQuery      string
	renderedQuery string
}

type queryRegistry struct {
	queries []*QueryInfo
}

func (r *queryRegistry) registerQuery(query string) queryID {
	if r.queries == nil {
		r.queries = make([]*QueryInfo, 0)
	}

	info := &QueryInfo{
		rawQuery: query,
	}

	r.queries = append(r.queries, info)
	return queryID(len(r.queries) - 1)
}

func (r *queryRegistry) getQueries() []*QueryInfo {
	return r.queries
}

func (r *queryRegistry) getQuery(id queryID) *QueryInfo {
	return r.queries[id]
}

// GetQuery returns copy of renderedQuery.
func (q *QueryInfo) GetQuery() string {
	return q.renderedQuery
}

var defaultQueryRegistry queryRegistry

func registerQuery(query string) queryID {
	return defaultQueryRegistry.registerQuery(query)
}

func query(id queryID) string {
	info := defaultQueryRegistry.getQuery(id)
	return info.renderedQuery
}
