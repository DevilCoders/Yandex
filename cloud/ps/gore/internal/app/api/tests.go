package api

import (
	"strings"
)

func mock() {
	// s, err := clients.GetServiceByName("pstest")
	// t := time.Now().Unix()
	// fmt.Println(s.ID.String())
	// shs, err := clients.GetShifts(t, s.ID.Hex(), true)
	// err = c.SetRuleForService(s, shs)

	// fmt.Println(err)
	// err = c.CloseTicket("CLOUDDUTY-1930")
}

// func init() {
// 	fieldsFromQuery("id,schedule.active")
// }

func fieldsFromQuery(q string) (fs [][]string) {
	rf := strings.Split(q, ",")
	for _, f := range rf {
		fs = append(fs, strings.Split(f, "."))
	}

	return
}
