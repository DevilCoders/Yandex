package pagination

import "a.yandex-team.ru/library/go/x/math"

const (
	MinPageSize     = 0
	DefaultPageSize = 100
	MaxPageSize     = 1000
)

type PageToken interface {
	HasMore() bool
}

type OffsetPageToken struct {
	Offset int64
	More   bool `json:"-"`
}

func (opt OffsetPageToken) HasMore() bool {
	return opt.More
}

func NewOffsetPageToken(newOffset int64) OffsetPageToken {
	return OffsetPageToken{
		Offset: newOffset,
		More:   true,
	}
}

/*
SanePageSize constrains specified page size to be in range (`MinPageSize`, `MaxPageSize`].
Returns:
- `DefaultPageSize` if constrained page size value is equal to 0.
- Constrained page size value, otherwise.
*/
func SanePageSize(pageSize int64) int64 {
	if pageSize > MaxPageSize {
		pageSize = MaxPageSize
	} else if pageSize <= MinPageSize {
		pageSize = DefaultPageSize
	}
	return pageSize
}

/*
SanePageOffset constrains specified offset to be not negative.
Returns:
- 0 if offset value is less than 0.
- offset value, otherwise.
*/
func SanePageOffset(offset int64) int64 {
	if offset <= 0 {
		return 0
	}
	return offset
}

type Page struct {
	Size           int64
	HasMore        bool
	NextPageOffset int64
	LowerIndex     int
	UpperIndex     int
}

// NewPage calculates pagination units based on total count of retrieved objects, specified page size and offset
func NewPage(totalCount, pageSize, offset int64) Page {
	pageSize = SanePageSize(pageSize)
	offset = SanePageOffset(offset)

	var (
		nextOffset     int64 = 0
		actualPageSize       = pageSize
		hasMore              = false
	)

	if pageSize+offset >= totalCount {
		actualPageSize = totalCount - offset
		nextOffset = totalCount
	} else {
		nextOffset = pageSize + offset
		hasMore = true
	}

	return Page{
		Size:           actualPageSize,
		HasMore:        hasMore,
		NextPageOffset: nextOffset,
		LowerIndex:     math.MinInt(int(offset), int(totalCount)),
		UpperIndex:     math.MinInt(int(offset+pageSize), int(totalCount)),
	}
}
