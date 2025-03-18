package model

type VersionMeta struct {
	Version int64
	Active  bool
}

const (
	defaultPageSize = 20
	maxPageSize     = 100
)

type Pagination struct {
	PageToken uint32
	PageSize  uint32
}

func (p *Pagination) Limit() int {
	if p.PageSize == 0 {
		return defaultPageSize
	}

	if int(p.PageSize) > maxPageSize {
		return maxPageSize
	}

	return int(p.PageSize)
}

func (p *Pagination) Offset() int {
	return int(p.pageToken()-1) * p.Limit()
}

func (p *Pagination) NextPageToken() uint32 {
	if p == nil {
		return 0
	}

	pageToken := p.pageToken()
	return pageToken + 1
}

func (p *Pagination) pageToken() uint32 {
	page := p.PageToken
	if page == 0 {
		page = 1
	}

	return page
}
