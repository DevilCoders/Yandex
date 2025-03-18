package main

import (
	"encoding/json"
	"fmt"
	"log"
	"strings"
)

// ChatMapping - Тип для мапинга чатов и сервисов
type ChatMapping struct {
	ServiceID           string `json:"service_id"`
	ChatID              int64  `json:"chat_id"`
	ConfigNotifications bool   `json:"config_notification"`
	ReleaseNotification bool   `json:"release_notification"`
	RulesNotification   bool   `json:"rules_notification"`
}

type AbcLogin struct {
	Login	string `json:"login"`
}

type AbcPerson struct {
	Person	AbcLogin `json:"person"`
}

type AbcResults struct {
	Results []AbcPerson `json:"results"`
}

type StaffTgLogin struct {
	TgLogin	string `json:"value_lower"`
}

type StaffTgPersonAccounts struct {
	Accounts []StaffTgLogin `json:"telegram_accounts"`
}

type StaffTgResults struct {
	Results []StaffTgPersonAccounts `json:"result"`
}

// RegisterBotChat - функция регистрации чата
func RegisterBotChat(chatInfo *ChatMapping) error {
	data, mErr := json.Marshal(chatInfo)
	if mErr != nil {
		log.Printf("Error while stringifing JSON\n%s\n", mErr.Error())
		return mErr
	}

	apiURL := fmt.Sprintf("%s%s", APIBaseURL, "register_bot_chat")
	_, err := APIRequest("POST", apiURL, data)
	if err != nil {
		log.Printf("No body\n%s\n", err.Error())
		return err
	}

	return nil
}

// GetInfo - функция получения информации о чатах
func GetInfo() (answer *ChatMapping, err error) {
	apiURL := fmt.Sprintf("%s%s", APIBaseURL, "get_bot_configs")
	body, err := APIRequest("GET", apiURL, nil)
	if err != nil {
		log.Printf("No body\n%s\n", err.Error())
		return nil, err
	}

	fmt.Println("Body:>", string(body))

	data := ChatMapping{}
	errMarsh := json.Unmarshal(body, &data)

	if errMarsh != nil {
		log.Printf("Error while parsing body\n%s\n", errMarsh.Error())
		return nil, errMarsh
	}

	return &data, nil
}

func GetAbcLogins() (answer string, err error) {
	body, err := APIAbcRequest()
	if err != nil {
		log.Printf("No body\n%s\n", err.Error())
		return "", err
	}

	data := AbcResults{}
	errMarsh := json.Unmarshal(body, &data)

	if errMarsh != nil {
		log.Printf("Error while parsing body\n%s\n", errMarsh.Error())
		return "", errMarsh
	}
	var b strings.Builder
	for _, p :=  range data.Results {
		fmt .Fprintf(&b, "%s,", p.Person.Login)
	}
	logins := b.String()
	logins = logins[:b.Len()-2]

	return logins, nil
}

func GetTgLoginsFromStaff() (answer []string, err error) {
	logins, err := GetAbcLogins()
	if err != nil {
		log.Printf("Error in GetAbcLogins\n%s\n", err.Error())
		return nil, err
	}

	body, err := APIStaffRequest(logins)

	if err != nil {
		log.Printf("No body\n%s\n", err.Error())
		return nil, err
	}
	data := StaffTgResults{}
	errMarsh := json.Unmarshal(body, &data)

	if errMarsh != nil {
		log.Printf("Error while parsing body\n%s\n", errMarsh.Error())
		return nil, errMarsh
	}

	var result []string

	for _, p :=  range data.Results {
		for _, a := range p.Accounts {
			result = append(result, a.TgLogin)
		}
	}

	return result, nil
}

func IsAllowedLogin(login string) (answer bool, err error) {
	tgLogins, err := GetTgLoginsFromStaff()
	if err != nil {
		return false, err
	}
	for _, elem := range tgLogins {
		if login == elem {
			return true, nil
		}
	}
	return false, nil
}
