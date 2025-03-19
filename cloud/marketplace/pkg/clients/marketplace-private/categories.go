package marketplace

type categoryManager interface {
	GetCategoryByIDorName(categoryID string) (*Category, error)
}

type Category struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	Type        string `json:"type"`
	Title       string `json:"title"`
	Description string `json:"description"`
	Rank        string `json:"rank"`
}

func (s *Session) GetCategoryByIDorName(categoryID string) (*Category, error) {
	var result Category

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetPathParam("categoryID", categoryID).
		Get("/marketplace/v2/private/categories/{categoryID}")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}
