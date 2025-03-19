package marketplace

type publisherManager interface {
	GetPublisherByID(publisherID string) (*Publisher, error)
}

type Publisher struct {
	ID        string `json:"id"`
	Name      string `json:"name"`
	State     string `json:"state"`
	VersionID string `json:"versionId"`
}

func (s *Session) GetPublisherByID(publisherID string) (*Publisher, error) {
	var result Publisher

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetPathParam("publisherID", publisherID).
		Get("/marketplace/v2/private/publishers/{publisherID}")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}
