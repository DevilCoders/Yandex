package main

import (
	"encoding/json"
	"fmt"
	"log"
)

// UrlDecrypt - ответ ручки расшифровки
type UrlDecrypt struct {
	Urls []string `json:"urls"`
}

// DecryptUrl - функция расшифровки урлов
func DecryptUrl(seviceID string, url string) (answer string, err error) {
	apiURL := fmt.Sprintf("%s%s%s", APIBaseURL, "decrypt_urls/", seviceID)
	req_data := UrlDecrypt{Urls: []string{url}}
	req_body, err := json.Marshal(req_data)
	if err != nil {
		log.Printf("Error while jsonify request body body\n%s\n", err.Error())
		return "", err
	}
	body, err := APIRequest("POST", apiURL, req_body)
	if err != nil {
		log.Printf("No body\n%s\n", err.Error())
		return "", err
	}

	data := UrlDecrypt{}
	errMarsh := json.Unmarshal(body, &data)

	if errMarsh != nil {
		log.Printf("Error while parsing body\n%s\n", errMarsh.Error())
		return "", errMarsh
	}
	if len(data.Urls) > 0 {
		return data.Urls[0], nil
	}
	return "", nil
}
