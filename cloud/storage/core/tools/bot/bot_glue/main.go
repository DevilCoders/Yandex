package main

import (
	"bufio"
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"github.com/spf13/cobra"
	"io/ioutil"
	"log"
	"math/rand"
	"net/http"
	"net/url"
	"os"
	"strconv"
	"strings"
	"time"
)

const oncallCommand string = "/oncall"

var lenOncallCommand int = len(oncallCommand)

const dutyCommand string = "/duty"

var lenDutyCommand int = len(dutyCommand)

const selfName string = "NBSExpertBot"
const botTag string = "@" + selfName

var lenBotTag int = len(botTag)

const telegramAPIBaseURL string = "https://api.telegram.org/bot"
const telegramTokenEnv string = "TELEGRAM_BOT_TOKEN"

var sendMessageURL string = telegramAPIBaseURL + os.Getenv(telegramTokenEnv) + "/sendMessage"
var sendStickerURL string = telegramAPIBaseURL + os.Getenv(telegramTokenEnv) + "/sendSticker"
var getUpdatesURL string = telegramAPIBaseURL + os.Getenv(telegramTokenEnv) + "/getUpdates"

type Update struct {
	UpdateID int     `json:"update_id"`
	Message  Message `json:"message"`
}

type GetUpdatesResponse struct {
	Updates []Update `json:"result"`
	Ok      bool     `json:"ok"`
}

func (u Update) String() string {
	return fmt.Sprintf("(update id: %d, message: %s)", u.UpdateID, u.Message)
}

type From struct {
	Username string `json:"username"`
}

type ReplyMessage struct {
	From From `json:"from"`
}

type Message struct {
	ID       int          `json:"message_id"`
	Text     string       `json:"text"`
	Chat     Chat         `json:"chat"`
	Audio    Audio        `json:"audio"`
	Voice    Voice        `json:"voice"`
	Document Document     `json:"document"`
	From     From         `json:"from"`
	ReplyTo  ReplyMessage `json:"reply_to_message"`
}

func (m Message) String() string {
	return fmt.Sprintf("(text: %s, chat: %s, audio %s)", m.Text, m.Chat, m.Audio)
}

type Audio struct {
	FileID   string `json:"file_id"`
	Duration int    `json:"duration"`
}

func (a Audio) String() string {
	return fmt.Sprintf("(file id: %s, duration: %d)", a.FileID, a.Duration)
}

type Voice Audio

type Document struct {
	FileID   string `json:"file_id"`
	FileName string `json:"file_name"`
}

func (d Document) String() string {
	return fmt.Sprintf("(file id: %s, file name: %s)", d.FileID, d.FileName)
}

type Chat struct {
	ID   int    `json:"id"`
	Type string `json:"type"`
}

func (c Chat) String() string {
	return fmt.Sprintf("(id: %d)", c.ID)
}

type Answer struct {
	Score   float64 `json:"score"`
	Text    string  `json:"answer"`
	Sticker string  `json:"sticker"`
}

func min(a int, b int) int {
	if a < b {
		return a
	}

	return b
}

func processMessage(
	s string,
	chatType string,
	isReply bool,
) (string, bool) {
	if len(s) >= lenOncallCommand {
		if s[:lenOncallCommand] == oncallCommand {
			return s[min(lenOncallCommand+1, len(s)):], true
		}
	}

	if len(s) >= lenDutyCommand {
		if s[:lenDutyCommand] == dutyCommand {
			s = s[min(lenDutyCommand+1, len(s)):]
		}
	}

	if strings.Contains(s, botTag) {
		return strings.ReplaceAll(s, botTag, ""), true
	}

	if chatType == "private" {
		return s, true
	}

	if isReply {
		return s, true
	}

	if rand.Float64() <= 0.005 {
		return s, true
	}

	return "", false
}

type options struct {
	Host          string
	Port          uint32
	BotName       string
	Verbose       bool
	UserWhitelist string
}

func selectAnswer(
	opts *options,
	login string,
	context string,
	userWhitelist map[string]string,
	chatID int,
) (string, string, error) {
	values := map[string]string{
		"name":    opts.BotName,
		"login":   login,
		"context": context,
		"chat_id": strconv.Itoa(chatID),
	}

	jsonValue, _ := json.Marshal(values)

	resp, err := http.Post(
		fmt.Sprintf("http://%s:%d/select_answer", opts.Host, opts.Port),
		"application/json",
		bytes.NewBuffer(jsonValue))

	if err != nil {
		log.Printf("error calling bot_brain api: %s", err.Error())
		return "", "", err
	}

	var answer Answer
	if err := json.NewDecoder(resp.Body).Decode(&answer); err != nil {
		log.Printf("could not decode incoming answer %s", err.Error())
		return "", "", err
	}

	// XXX very inefficient hack
	for tgLogin, login := range userWhitelist {
		answer.Text = strings.ReplaceAll(answer.Text, login, "@"+tgLogin)
	}

	defer resp.Body.Close()
	return answer.Text, answer.Sticker, nil
}

func getUpdates(offset int) ([]Update, error) {
	log.Printf("getting updates at offset %d", offset)

	values := map[string]interface{}{
		"offset": offset}

	jsonValue, _ := json.Marshal(values)

	resp, err := http.Post(
		getUpdatesURL,
		"application/json",
		bytes.NewBuffer(jsonValue))

	if err != nil {
		log.Printf("error getting updates: %s", err.Error())
		return nil, err
	}

	defer resp.Body.Close()
	respBody, _ := ioutil.ReadAll(resp.Body)

	log.Print(string(respBody))
	var result GetUpdatesResponse
	if err := json.Unmarshal(respBody, &result); err != nil {
		log.Printf("could not decode updates: %s", err.Error())
		return nil, err
	}

	return result.Updates, nil
}

func sendReply(chatID int, text string, origID int) (string, error) {
	log.Printf("Sending %s to chat_id: %d", text, chatID)
	resp, err := http.PostForm(
		sendMessageURL,
		url.Values{
			"chat_id":             {strconv.Itoa(chatID)},
			"text":                {text},
			"reply_to_message_id": {strconv.Itoa(origID)},
			"__parse_mode":        {"Markdown"},
		})

	if err != nil {
		log.Printf("error posting text to the chat: %s", err.Error())
		return "", err
	}

	defer resp.Body.Close()

	var bodyBytes, errRead = ioutil.ReadAll(resp.Body)
	if errRead != nil {
		log.Printf("error parsing telegram answer %s", errRead.Error())
		return "", err
	}

	bodyString := string(bodyBytes)
	log.Printf("telegram response body: %s", bodyString)

	return bodyString, nil
}

func sendSticker(chatID int, stickerID string) (string, error) {
	log.Printf("Sending sticker %s to chat_id: %d", stickerID, chatID)
	resp, err := http.PostForm(
		sendStickerURL,
		url.Values{
			"chat_id": {strconv.Itoa(chatID)},
			"sticker": {stickerID},
		})

	if err != nil {
		log.Printf("error posting sticker to the chat: %s", err.Error())
		return "", err
	}

	defer resp.Body.Close()

	var bodyBytes, errRead = ioutil.ReadAll(resp.Body)
	if errRead != nil {
		log.Printf("error parsing telegram answer %s", errRead.Error())
		return "", err
	}

	bodyString := string(bodyBytes)
	log.Printf("telegram response body: %s", bodyString)

	return bodyString, nil
}

func run(ctx context.Context, opts *options) error {
	//level := nbs.LOG_INFO
	//if opts.Verbose {
	//	level = nbs.LOG_DEBUG
	//}

	userWhitelist := map[string]string{}
	{
		file, err := os.Open(opts.UserWhitelist)
		if err != nil {
			return err
		}
		defer file.Close()

		scanner := bufio.NewScanner(file)
		for scanner.Scan() {
			words := strings.Fields(scanner.Text())
			userWhitelist[words[0]] = words[1]
		}

		if err := scanner.Err(); err != nil {
			return err
		}
	}

	var offset int
	for {
		var updates, err = getUpdates(offset)
		if err != nil {
			log.Printf("getUpdates error: %s", err.Error())
		} else {
			for _, update := range updates {
				if update.UpdateID+1 > offset {
					offset = update.UpdateID + 1
				}

				log.Printf("message from: %s", update.Message.From.Username)
				login := userWhitelist[update.Message.From.Username]
				if len(login) == 0 {
					log.Printf("user not in whitelist")
					continue
				}

				if len(update.Message.Text) > 0 {
					isReply := false
					if update.Message.ReplyTo.From.Username == selfName {
						isReply = true
					}
					var text, shouldReact = processMessage(
						update.Message.Text,
						update.Message.Chat.Type,
						isReply)
					if shouldReact {
						var answer, sticker, err = selectAnswer(
							opts,
							login,
							text,
							userWhitelist,
							update.Message.Chat.ID,
						)
						if err != nil {
							log.Printf("selectAnswer error: %s", err.Error())
						} else {
							if len(sticker) > 0 {
								var _, err = sendSticker(
									update.Message.Chat.ID,
									sticker)

								if err != nil {
									log.Printf("sendSticker error: %s", err.Error())
								}
							}

							var _, err = sendReply(
								update.Message.Chat.ID,
								answer,
								update.Message.ID)

							if err != nil {
								log.Printf("sendReply error: %s", err.Error())
							}
						}
					}
				}
			}
		}

		time.Sleep(time.Second)
	}
}

func main() {
	var opts options

	rootCmd := cobra.Command{
		Use: "bot_glue",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().StringVar(&opts.Host, "host", "localhost", "bot_brain server")
	rootCmd.Flags().Uint32Var(&opts.Port, "port", 11111, "bot_brain port")
	rootCmd.Flags().StringVar(&opts.BotName, "name", "prod_support", "bot name")
	rootCmd.Flags().StringVar(&opts.UserWhitelist, "user-whitelist", "user_whitelist", "path to a file with user whitelist")
	rootCmd.Flags().BoolVar(&opts.Verbose, "verbose", false, "verbose mode")

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
