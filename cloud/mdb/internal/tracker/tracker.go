package tracker

import "context"

type Comment struct {
	Text string
}

// API is Tracker API
type API interface {
	CreateComment(ctx context.Context, issueID string, comment Comment) (string, error)
	UpdateComment(ctx context.Context, issueID, commentID string, comment Comment) error
}
