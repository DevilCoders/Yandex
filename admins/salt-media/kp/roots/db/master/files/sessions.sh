#! /bin/bash
mysql -NBe 'select concat("recent ", count(DISTINCT user_id)) from user_last_visit WHERE make_date >= DATE_SUB(NOW(), INTERVAL 5 MINUTE)' kinopoisk
