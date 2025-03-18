package xiva

import (
	"context"
)

type SubscriptionSign struct {
	Sign      string `json:"sign"`
	Timestamp string `json:"ts"`
}

type Event struct {
	Payload interface{} `json:"payload"`
	Keys    interface{} `json:"keys"`
}

// Subscription is a client with an active subscription
type Subscription struct {
	ID      string `json:"id"`
	Client  string `json:"client"`
	Filter  string `json:"filter"`
	Session string `json:"session"`
	TTL     int    `json:"ttl"`
	URL     string `json:"url"`
}

type Client interface {
	GetSubscriptionSign(ctx context.Context, userID string) (SubscriptionSign, error)
	GetWebsocketURL(ctx context.Context, userID, sessionID, clientName string, filter *Filter) (string, error)
	SendEvent(ctx context.Context, userID, eventID string, event Event) error
	// ListActiveSubscriptions returns a list of active subscriptions for given user https://console.push.yandex-team.ru/#api-reference-list
	ListActiveSubscriptions(ctx context.Context, userID string) ([]Subscription, error)
}
