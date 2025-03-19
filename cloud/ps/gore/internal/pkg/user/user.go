package user

import "fmt"

const (
	mailDomain   = "yandex-team.ru"
	defaultStart = 32400 // 9:00
	defaultEnd   = 64800 // 18:00
)

type WorkDay struct {
	Start int
	End   int
}

type Notify struct {
	Abuse   bool
	Support bool
	Duty    bool
}

type Telegram struct {
	Active  bool
	TgID    int64
	Notify  Notify
	WorkDay WorkDay
	IsAdmin bool // WTF is this?!
}

type Mail struct {
	Active  bool
	Address string // Если вдруг адрес должен отличаться от l@y-t.ru
	Message string // Тело сообщения
	Subject string // Тема
}

type Calendar struct {
	Active     bool
	Attendance string //Отображение: "available" | "maybe" | "busy"
}

type User struct { //Конфиг для пользователя
	Active      bool
	Aliases     []string
	Calendar    *Calendar
	Mail        *Mail
	Telegram    *Telegram
	Username    string
	Permissions []string
}

func New(u string) *User {
	return &User{
		Active: true,
		Mail: &Mail{
			Address: fmt.Sprintf("%s@%s", u, mailDomain),
		},
		Telegram: &Telegram{
			Active: false,
			WorkDay: WorkDay{
				Start: defaultStart,
				End:   defaultEnd,
			},
		},
		Username: u,
	}
}
