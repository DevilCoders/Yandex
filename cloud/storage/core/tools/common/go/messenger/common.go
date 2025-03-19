package messenger

import (
	"context"
	"strings"
)

type MessengerType int32

const (
	Telegram MessengerType = iota
	QMssngr                = iota
)

type MessengerClientIface interface {
	SendMessage(ctx context.Context, chatID string, text string) error

	GetMessengerType() MessengerType

	GetUserChatID(username string) string
	GetUsersFile() string
}

////////////////////////////////////////////////////////////////////////////////

func getUserChatID(username string, userlist string) string {
	for _, line := range strings.Split(userlist, "\n") {
		parts := strings.Split(line, " ")
		if len(parts) == 0 {
			continue
		}

		if parts[0] == username {
			if len(parts) > 1 {
				return parts[1]
			}
			break
		}
	}

	return ""
}
