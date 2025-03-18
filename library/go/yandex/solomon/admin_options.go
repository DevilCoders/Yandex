package solomon

type (
	ListOption interface{ IsListOption() }

	PageSize struct {
		// If Size == nil, pageSize == "all"
		Size *int
		Page int
	}

	PageToken struct {
		Token string
	}

	AlertFilter struct {
		FilterByName string
	}

	// NextPageToken returns page token from the server.
	NextPageToken struct {
		NextToken string
	}
)

func (p PageSize) IsListOption() {}

func WithPageSize(size *int) PageSize {
	return PageSize{Size: size}
}

func (t PageToken) IsListOption() {}

func WithPageToken(token string) PageToken {
	return PageToken{Token: token}
}

func (f AlertFilter) IsListOption() {}

func (p *NextPageToken) IsListOption() {}
