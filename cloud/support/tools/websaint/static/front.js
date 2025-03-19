function senduserdata() {
    var data = document.getElementById('data');
    var radio = document.getElementsByName("typeID")
    var txt = "";
    var i;
    for (i = 0; i < radio.length; i++) {
    if (radio[i].checked) {
      txt = radio[i].value;
    }
    }

    var request = new XMLHttpRequest();
    var myHeaders = new Headers();
    request.open('GET', document.URL+'billing', true);
    document.getElementById('data').innerHTML = '<img src="/static/load.gif" alt="Loading" title="Loading" height="50" width="50" />'
    request.setRequestHeader("Access-Control-Allow-Origin", "*");
    request.setRequestHeader("X-AnyID", document.getElementById("anyID").value);
    request.setRequestHeader("X-Type", txt);
    request.send();

    request.onload = function () {
        document.getElementById('data').innerHTML = request.responseText;
    }
}

function print(data) {
    console.log(data);
}

function validateFormOld() {
    var services = document.getElementById('services').value.split(';');
    function check(element) {
        if (document.getElementById(element).checked) {
        return 13.208333333;
        }
        else {
        return 1;
        }
    }
    var quotasdict = {};
    var arraypath = window.location.pathname.split('/');
    try {
        quotasdict['cloudId'] = document.getElementById("anyID").value;
    }
    catch (e){
        quotasdict['cloudId'] = arraypath[arraypath.length - 1];
    }
    var summ=0;
    console.log(services);
    for (var service in services) {
        var servicename = services[service];
        quotasdict[servicename] = {};
        quotavalues = document.getElementsByName(services[service]);
        for (var quota = 0; quota < quotavalues.length; quota++) {
            summ += document.getElementById(quotavalues[quota].id).value * document.getElementById(quotavalues[quota].id+'-ph').value;
        }
    }
    var grant = parseInt(document.getElementById("grant").value);
    var crlimit = parseInt(document.getElementById("cr-limit").value);
    var balance = parseInt(document.getElementById("balance").value);
    var totalbl = grant + crlimit + balance;

    var currencyreq = new XMLHttpRequest();
    currencyreq.responseType = 'json';
    currencyreq.open('GET', 'https://www.cbr-xml-daily.ru/daily_json.js'+'?'+(new Date()).getTime(), true);
    currencyreq.send(null);
    currencyreq.onload = function () {
        var json = currencyreq.response;
        var curusd = 1;
        var cur = document.getElementById("currency").textContent;
        if (cur == 'USD') {
            curusd = parseFloat(json['Valute']['USD']['Value']);
        }
        document.getElementById('price-ph').innerHTML = "Стоимость в час: " + Math.round(summ/curusd) + cur;
        document.getElementById('price-p2d').innerHTML = "Стоимость в 2 дня: " + Math.round(summ * 48 / curusd) + cur;
        document.getElementById('total-bl').innerHTML = "Баланс + кредит + грант: " + totalbl + cur;

        var summ48 = summ * 48 / curusd;
        if ( totalbl >= summ48 ) {
             document.getElementById('decision').innerHTML = "Не требуется пополнение";
             document.getElementById('decision').setAttribute("class", "true");
        } else {
             document.getElementById('decision').innerHTML = "Если думаешь, что пришёл фродер - можно попросить пополнить счёт на эту сумму";
             document.getElementById('decision').setAttribute("class", "false");
             document.getElementById('wbepaid').innerHTML = "На сумму: " + Math.abs(totalbl - Math.round(summ * 48 / curusd)) + cur +" +запас " + Math.round(summ * 2 / curusd) + cur;
        }
//        document.getElementById('getcode').hidden = false;
    };
}

function validateForm() {
    document.getElementById('warning').innerHTML = '';
    document.getElementById('deny').innerHTML = '';
    var money = parseInt(document.getElementById("money").value);
    var supportType = parseInt(document.getElementById("support").value);
    var quotasdict = {};
    var arraypath = window.location.pathname.split('/');
    try {
        quotasdict['cloudId'] = document.getElementById("anyID").value;
    }
    catch (e){
        quotasdict['cloudId'] = arraypath[arraypath.length - 1];
    }
    var services = document.getElementById('services').value;
    services = services.split(';');
    var alertmessage = [];
    var denylist = [];

    for (var service in services) {
        var servicename = services[service];
        quotasdict[servicename] = {};
        quotavalues = document.getElementsByName(services[service]);
        for (var quota = 0; quota < quotavalues.length; quota++) {
                var quotaPresent = quotavalues[quota].id.split('_')[0].trim();
                var quotaToSet = quotavalues[quota].id.trim();
                var quotaPresentValue = parseInt(document.getElementById(quotaPresent).value);
                var quotaToSetValue = parseInt(document.getElementById(quotaToSet).value);
                var approved =  document.getElementById(quotaPresent+'_approved').checked;
                if ( quotaPresentValue != quotaToSetValue ) {
                    try {
                        var quotaHardLimit = parseInt(limits[quotaPresent]["safety_limit"]);
                        var quotaPremLimit = parseFloat(limits[quotaPresent]["enterprise_limit"]);
                        var quotaBaseLimit = parseFloat(limits[quotaPresent]["base_limit"]);
                        var quotaApprovers = limits[quotaPresent]["approvers"];
                        if ( supportType == 2 || supportType == 3 ) {
                            var soft = quotaPremLimit;
                        }
                        else {
                            var soft = quotaBaseLimit;
                        }
                        if ( quotaToSetValue > quotaHardLimit && !(approved) ) {
                            document.getElementById(quotaToSet).parentNode.style.backgroundColor = "red";
                            denylist.push(`${quotaPresent} — согласующие ${quotaApprovers}`);
                            continue;
                        }
                        if ( quotaToSetValue > soft && !(approved) ) {
                            document.getElementById(quotaToSet).parentNode.style.backgroundColor = "yellow";
                            alertmessage.push(`${quotaPresent}`)
                            validateFormOld()
                            continue;
                        }

                        quotasdict[servicename][quotaPresent] = parseInt(quotaToSetValue);
                    }
                    catch (err) {
                        console.log('error: '+err+', '+quotaToSet);
                    }
                }
                else {
                    continue;
                }
                console.log(quotaToSet.id, quotaToSetValue);
        }
    }
    console.log(alertmessage.length);
    if ( alertmessage.length > 0 ) {
        document.getElementById('warning').innerHTML = "Следующие квоты не будут увеличены<br>"+alertmessage.join('<br>')+"<br>Поставь компоненту 'вернули' чтобы поддержка увидела тикет"
    }
    if ( denylist.length > 0 ) {
        document.getElementById('deny').innerHTML = "Следующие квоты не будут увеличены<br>"+denylist.join('<br>')+"<br>Требуется согласование, верните тикет."
    }
    document.getElementById('link').hidden = false;
    document.getElementById('copy').hidden = false;
    document.getElementById('token').hidden = false;
    document.getElementById('labeltoken').hidden = false;
    document.getElementById('setquotas').hidden = false;
    var request = new XMLHttpRequest();
    var myHeaders = new Headers();
    request.open('POST', '/short', true);
    request.setRequestHeader("Content-type", "application/json");
    request.onreadystatechange = function () {
        if (request.readyState === 4 && request.status === 200) {
            var json = JSON.parse(request.responseText);
            document.getElementById('link').value = json.link;
        };
     }
     var data = JSON.stringify(quotasdict);
     request.send(data);
}


function copyLink() {
  document.getElementById('link').select();
  document.execCommand("copy");
}


function setquotas() {
    var arraypath = window.location.pathname.split('/');
    try {
        var cloud = document.getElementById("anyID").value;
    }
    catch (e){
        var cloud = arraypath[arraypath.length - 1];
    }
    var token = document.getElementById('link').value;
    var oauth = document.getElementById("token").value;
    var referrer = document.referrer;

    var request = new XMLHttpRequest();
    var myHeaders = new Headers();
    request.open('POST', '/setquota', true);
    document.getElementById('data').innerHTML = '<img src="/static/load.gif" alt="Loading" title="Loading" height="50" width="50" />'
    request.setRequestHeader("X-Oauth", oauth);
    request.setRequestHeader("X-Ticket", referrer);
    request.setRequestHeader("X-Token", token);
    request.setRequestHeader("X-Cloud", cloud);

    request.send();
    request.onload = function () {
        if (request.status != 200) {
            window.alert(`Что-то пошло не так. Токен запроса ${token}`);
            console.log(request.status);
            document.getElementById('data').innerHTML = request.status;
        }
        else {
            console.log(request.status);
            services = JSON.parse(request.responseText);
            document.getElementById('data').innerHTML = request.status+'<br>';
            for ( response in services ) {
                var resservirce = document.createElement('p');
                document.getElementById('data').innerHTML += response +' :'+ services[response] + '<br>' ;
            }
        }
     }

}

document.onload()
