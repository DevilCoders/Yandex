package main

import (
	"encoding/json"
	"fmt"
	"log"
)

// ArgusRun - ответ ручки запуска
type ArgusRun struct {
	ID    string `json:"id"`
	RunID int32  `json:"run_id"`
}

// RunArgus - функция запуска аргуса
func RunArgus(seviceID string) (answer *ArgusRun, err error) {
	apiURL := fmt.Sprintf("%s%s%s", APIBaseURL, seviceID, "/sbs_check/run")
	body, err := APIRequest("POST", apiURL, nil)
	if err != nil {
		log.Printf("No body\n%s\n", err.Error())
		return nil, err
	}

	fmt.Println("Body:>", string(body))

	data := ArgusRun{}
	errMarsh := json.Unmarshal(body, &data)

	if errMarsh != nil {
		log.Printf("Error while parsing body\n%s\n", errMarsh.Error())
		return nil, errMarsh
	}

	return &data, nil
}
