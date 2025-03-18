package main

import (
	"fmt"
	"log"
	"strconv"
	"strings"
	"time"

	tb "gopkg.in/tucnak/telebot.v2"
)

// InitBot - Инициализация бот
func InitBot() (bot *tb.Bot, e error) {
	bot, err := tb.NewBot(tb.Settings{
		// https://yav.yandex-team.ru/secret/sec-01f0axb1rj27gfn87qckm6vqbh/explore/versions
		Token:       GetEnv("TELEGRAM_TOKEN", ""),
		Poller:      &tb.LongPoller{Timeout: 30 * time.Second},
		Synchronous: false,
	})
	if err != nil {
		return nil, err
	}
	log.Printf("Telegram bot connected")
	return bot, nil
}

// InitLongPoller - Чтобы заработал polling нужно оторвать webhook
func InitLongPoller(bot *tb.Bot) error {
	if err := bot.RemoveWebhook(); err != nil {
		return fmt.Errorf("unable to remove webhook info: %w", err)
	}
	return nil
}

// InitHandlers - Навешиваем хендлеры
func InitHandlers(bot *tb.Bot) {
	bot.Handle("/hello", func(m *tb.Message) {
		log.Printf("%#v", m)
		bot.Send(m.Sender, "Hello World!")
	})

	bot.Handle("/register", func(m *tb.Message) {
		chatID := m.Chat.ID
		payload := strings.Split(m.Payload, " ")

		if len(payload) < 1 {
			bot.Send(m.Sender, "Error, need to know ServiceID")
			return
		}

		serviceID := payload[0]
		configNotification, _ := strconv.ParseBool(payload[1])
		releaseNotification, _ := strconv.ParseBool(payload[2])
		rulesNotification, _ := strconv.ParseBool(payload[3])

		chatInfo := ChatMapping{serviceID, chatID, configNotification, releaseNotification, rulesNotification}

		err := RegisterBotChat(&chatInfo)
		if err != nil {
			log.Printf("Error registration\n%s\n", err.Error())
			bot.Send(m.Sender, err.Error())
			return
		}

		bot.Send(m.Sender, "Done")
	})
}

// StartBot - Запускаем Бота
func StartBot(bot *tb.Bot) {
	bot.Start()
}

func main() {
	fmt.Println("Hello AAB")
	fmt.Println(GetServiceTicketForConfigsApi())

	// url, err := DecryptUrl("yandex_news", "/news/_nzp/td4t4tl73/8ebd419y/Bj9w8dPGQnCs8gmKtn861zBcEPmZXtCrxasVscdmO1gbMv_ZgBC0zabDTT5dFErWPCgZhIZNl7Ij0")
	// b, err := InitBot()

	// if err != nil {
	// 	log.Printf("Error while creating bot\n%s\n", err.Error())
	// 	return
	// }

	// InitLongPoller(b)
	// InitHandlers(b)
	// StartBot(b)

	// result, err := RegisterBotChat()
	// b, err := InitBot()

	// if err != nil {
	// 	log.Printf("Error while creating bot\n%s\n", err.Error())
	// 	return
	// }

	// InitLongPoller(b)
	// InitHandlers(b)
	// StartBot(b)

	result, err := GetInfo()
	if err != nil {
		log.Printf("Error while fetching info\n%s\n", err.Error())
	}
	log.Println(result)
	// log.Printf("Decrypt url: %s\n", url)

	// _, err := RunArgus("yandex_news")
	// if err != nil {
	// 	log.Printf("Error while running argus\n%s\n", err.Error())
	// }

	// log.Println(result)

	// _, err := RunArgus("yandex_news")
	// if err != nil {
	// 	log.Printf("Error while running argus\n%s\n", err.Error())
	// }
}
