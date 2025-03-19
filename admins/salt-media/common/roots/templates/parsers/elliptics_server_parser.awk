#!/usr/bin/mawk -f

function print_timings(name, array, suffix){
  # можно конечно отказаться от 'match_by' и пробежаться по массиву используя
  # форму обращения к массиву 2ой размерности типа arr[idx1, idx2]
  # но в этом случае для вывода суммарных значений придется использовать
  # отдельный код и честно говоря смысла в этом мало, потому-что способ
  # описанный ниже работает так же быстро (и даже на несколько наносекунд быстрее)
  # PS: в текущем случае (тайминги упаковывыаются по значению) отказаться от
  #     match_by не получиться ;-)
  match_by = name""SUBSEP
  # Multimetrics понимает упакованные тайминги
  # если в начале имени метрки есть @
  printf "@"name"_"suffix;
  for(i in array){
    if (index(i, match_by) != 0) {
      # <значение тайминга>@<кол-во>
      # при сборе таймингов от них уже отрезали 3 последние микросекунды
      # поэтому теперь мы имеем грубо округленные милисекунды,
      # делить теперь нужно на 1000
      printf " %.3f@%s", substr(i, index(i, SUBSEP)+1)/1e3, array[i]
    }
  }
  print "";
}

BEGIN {

metrics["READ_client_size"] = 0
metrics["WRITE_client_size"] = 0
metrics["READ_CACHE_client_size"] = 0
metrics["WRITE_CACHE_client_size"] = 0


}

$4 ~ /(^INFO|^ERROR)/ && ($6 ~ /[A-Z][A-Z][A-Z]/ || $7 ~ /client/) {
  # trim last ':' substr faster then gsub
  op = substr($6, 1, length($6)-1)
  type = substr($7, 1, length($7)-1)

  if ($NF == "1351]")
    op = op"_CACHE"

  if ($14 == "time:") {
    err = "_"$21
    time = $15
    queue_time = $18

  } else if ($27 == "time:"){
    err = "_"$33
    time = $28
    queue_time = $31

    size = substr($20, 1, index($20, "/")-1)
    metrics[op"_"type"_size"] += size/1e6

  } else { next }  # go to next line without counting

  operations[op]=1  # awareness about timings sub arrays

  #if (op == "ROUTE_LIST")
    #print $0
  # время в логе представлено в usec, то есть это 1/1,000,000 секунды
  # так как время в логе будет представлено в виде милисекунд,
  # можно полностью игнорировать 3 последние цыфры
  # потому-что они не повлияют на результат округления до милисекунд
  timings[op, substr(time, 1, length(time)-3)]++
  queue_timings[op, substr(queue_time, 1, length(queue_time)-3)]++
  metrics[op""err]++
}

END {
  for(c in metrics){
    k = c; gsub(/[\[\]':]+/, "", k);gsub(/_$/, "", k)
    print k, metrics[c]
  }


  # print packed timings, multimetrics know about packed mode
  for(o in operations){
    print_timings(o, timings, "timings")
    print_timings(o, queue_timings, "queue_timings")
  }
}
