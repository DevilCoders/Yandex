package main

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
)

// APIBaseURL - Адрес админки антиадблока
var ConfigsApiHost = GetEnv("AAB_ADMIN_HOST", "preprod.aabadmin.yandex.ru")
var APIBaseURL = fmt.Sprintf("https://%s/", ConfigsApiHost)
// роли разработчик, системный администратор, поддерживающий
var APIAbcURL = "https://abc-back.yandex-team.ru/api/v4/services/members/?fields=person.login&is_robot=false&service=1526"
// нужно доклеить логины через запятую из ответа abc
var APIStaffBaseURL = "https://staff-api.yandex-team.ru/v3/persons?_fields=telegram_accounts.value_lower&login="


func MakeRequest(req http.Request) (answer []byte, err error) {
	client := &http.Client{}
	resp, err := client.Do(&req)
	if err != nil {
		log.Printf("Error while doing request\n%s\n", err.Error())
		return nil, err
	}
	defer resp.Body.Close()
	fmt.Println("Status:>", resp.Status)
	fmt.Println("Headers:>", resp.Header)
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		log.Printf("Error while reading body\n%s\n", err.Error())
		return nil, err
	}
	fmt.Println("Body:>", string(body))
	return body, nil
}

// APIRequest - запросы в админку
func APIRequest(method string, url string, data []byte) (answer []byte, err error) {
	req, err := http.NewRequest(method, url, bytes.NewBuffer(data))
	if err != nil {
		log.Printf("Error while creating request\n%s\n", err.Error())
		return nil, err
	}
	secret := GetServiceTicketForConfigsApi()
	req.Header.Set("X-Ya-Service-Ticket", secret)
	body, err := MakeRequest(*req)
	return body, err
}


func APIAbcRequest() (answer []byte, err error) {
	req, err := http.NewRequest("GET", APIAbcURL, nil)
	if err != nil {
		log.Printf("Error while creating request\n%s\n", err.Error())
		return nil, err
	}
	token := GetEnv("TOOLS_TOKEN", "")
	req.Header.Set("Authorization", fmt.Sprintf("OAuth %s", token))
	body, err := MakeRequest(*req)
	return body, err
}


func APIStaffRequest(logins string) (answer []byte, err error) {
	url := fmt.Sprintf("%s%s", APIStaffBaseURL, logins)
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		log.Printf("Error while creating request\n%s\n", err.Error())
		return nil, err
	}
	token := GetEnv("TOOLS_TOKEN", "")
	req.Header.Set("Authorization", fmt.Sprintf("OAuth %s", token))
	body, err := MakeRequest(*req)
	return body, err
}
