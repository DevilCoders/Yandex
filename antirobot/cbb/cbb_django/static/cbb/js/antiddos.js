function check_antiddos() {
    let rules = document.getElementById("id_rng_txt").value;
    let csrftoken = $("[name=csrfmiddlewaretoken]").val();
    let request = document.getElementById("ddos-check-request").value;
    document.getElementById("ddos-check-result").innerHTML = "<span class=\"bg-info\">loading</span>";

    $.ajax({
        type: "POST",
        url: "/ajax/ajax_antiddos_check",
        dataType: "json",
        async: true,
        data: {
            rules: rules,
            request: request,
        },
        headers: {
            "X-CSRFToken": csrftoken
        },
        success: function (data) {
            let result = "Syntax <span class=\"bg-danger\">ERROR</span>";

            if (!data.correct_length) {
                result = "Rule <span class=\"bg-danger\">TOO LONG ( > 1024)</span>";
            }

            document.getElementById("submit").disabled = true;


            if (data.correct_length && data.correct_syntax) {
                result = "Syntax <span class=\"bg-success\">OK</span>"
                document.getElementById("submit").disabled = false;

                if (data.request_valid) {
                    result += " Request <span class=\"bg-success\">VALID</span>"
                    if (data.request_matches) {
                        result += " and <span class=\"bg-success\">MATCHES</span>"

                        let ignored_checks;

                        if ("ignored_checks" in data) {
                            ignored_checks = data.ignored_checks;
                        } else {
                            ignored_checks = ["ip_from"];
                        }

                        if (ignored_checks.length > 0) {
                            let ignored_checks_str = ignored_checks.join(", ");
                            let verb = ignored_checks.length > 1 ? "have" : "has";
                            result +=
                                ` (${ignored_checks_str} ${verb} been excluded from request ` +
                                `matching)`;
                        }
                    } else {
                        result += " and <span class=\"bg-danger\">NOT MATCHING</span>"
                    }
                } else {
                    result += " Request <span class=\"bg-danger\">INVALID</span>"
                }
            }

            document.getElementById("ddos-check-result").innerHTML = result;
        }
    });
}

function fill_controls(data) {
    if (!data) {
        return;
    }

    var service_select = document.getElementById("service_type");

    Object.keys(data).forEach(function(key) {
        var opt = document.createElement("option");
        opt.value = key;
        opt.innerHTML = key;
        service_select.appendChild(opt);
    });

    service_select.style = "";

    window.randomRequests = data;
}

function get_random_requests() {
    let csrftoken = $("[name=csrfmiddlewaretoken]").val();
    document.getElementById("yt_loading").innerHTML = "Загрузка из <a href=\"https://yt.yandex-team.ru/hahn/navigation?path=//home/antirobot/log-viewer/cbb_requests_services\">YT Hahn</a>...";
    document.getElementById("service_type").style = "display: none";
    document.getElementById("request_num").style = "display: none";

    $.ajax({
        type: "POST",
        url: "/ajax/ajax_get_random_requests",
        dataType: "json",
        async: true,
        data: {},
        headers: {
            "X-CSRFToken": csrftoken
        },
        success: function (data) {
            fill_controls(data);
            document.getElementById("yt_loading").innerHTML = "";
        }
    });
}

function select_service() {
    var service_select = document.getElementById("service_type");
    var request_select = document.getElementById("request_num");
    var requests = window.randomRequests;

    var service = service_select.options[service_select.selectedIndex].value;

    request_select.innerHTML = "<option>Выберите запрос</option>";

    requests[service].forEach(function(elem, index) {
        var opt = document.createElement("option");
        opt.value = elem;
        opt.innerHTML = index;
        request_select.appendChild(opt);
    });

    request_select.style = "";
}

function select_request() {
    var request_select = document.getElementById("request_num");

    var request = request_select.options[request_select.selectedIndex].value;

    document.getElementById("ddos-check-request").value = request;
}


$(document).ready(function () {
    if (location.pathname.match(/groups\/77\//)) {
        jQuery("[id*=hint-row]").remove();
        jQuery("#check").remove();
        return;
    }

    document.getElementById("id_rng_txt").value = "doc=/\\/search\\.*/;\
header['X-Forwarded-For-Y']=/1.2.3.*/;\
header['X-Antirobot-Service-Y']=/cbb/;\
header['X-Yandex-Ja3']=/771,49200-49196/\
";
    document.getElementById("ddos-check-request").value = `GET /search/?text=котики HTTP/1.1
Host: yandex.ru
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.71 Safari/537.17 YE
Accept-Encoding: gzip,deflate,sdch
Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4
Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.3
Cookie: yandexuid=9734851951328192577
X-Antirobot-Service-Y: cbb
X-Forwarded-For-Y: 1.2.3.4
X-Source-Port-Y: 32736
X-Start-Time: 1391611472705260
X-Req-Id: 1391611472705260-17432647174108823325
X-Yandex-Ja3: 771,49200-49196
`
    document.getElementById("id_rng_txt").setAttribute("onchange", "check_antiddos()");
    document.getElementById("ddos-check-request").setAttribute("onchange", "check_antiddos()");

    document.getElementById("service_type").setAttribute("onchange", "select_service()");
    document.getElementById("request_num").setAttribute("onchange", "select_request()");
});
