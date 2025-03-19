#!/bin/bash
# Get environment type
if [ -f "/etc/yandex/environment.type" ]; then
        ENVIR=`cat /etc/yandex/environment.type`
else
        echo '2;No /etc/yandex/environment.type, exit'
fi

if [  $ENVIR == "production" ] || [ $ENVIR == "prestable" ]; then
       url="http://weather-elliptics.weather.yandex.net/get/weather-embed/forecasts-by-geo/213.xml"
elif [ $ENVIR == "testing"  ]; then 
       url="http://weather-elliptics-test.weather.yandex.net/get/weather-embed/forecasts-by-geo/213.xml"
else 
       echo "2;Uknown environment; exit" 
fi

cl=$(curl --connect-timeout 5 -m 5 $url  2>/dev/null)
if  [ $? -ne "0" ]; then 
    echo "2;Connect to elliptics fail"
    exit ;
fi
date1=$(echo "$cl" | grep "<uptime>" | head -1 | sed 's/<uptime>//g'  | sed 's/<\/uptime>//g'  | sed 's/T/ /g')
date1=$(date --date="$date1" +"%s")
date2=$(date +"%s")
diff_minutes=$[ ($date2 - $date1) / 60 ]
if [ -z "$diff_minutes" ] ; then 
   echo "2;forecasts-by-geo/213.xml Xml is wrong" 
   exit;
fi
if [ $diff_minutes -gt 30 ]; then
  echo "2;Time diffrent with elliptics data and now is $diff_minutes "
else
 echo "0;OK. Time diffrent with elliptics data and now is $diff_minutes "
fi

