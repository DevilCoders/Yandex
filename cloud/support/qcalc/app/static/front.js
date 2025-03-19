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



function validateForm() {
    var types= ['compute', 'mdb']
    function check(element) {
        if (document.getElementById(element).checked) {
        return 13.208333333;
        }
        else {
        return 1;
        }
    }

    var quotas = document.getElementsByName('compute');
    for (var quota in quotas) {
            if (quotas[quota].id != undefined) {
               quotas[quota].value = math.evaluate(quotas[quota].value) * parseInt(document.getElementById('computemult').value);
            }
    }


    var quotas = document.getElementsByName('mdb');
    for (var quota in quotas) {
            if (quotas[quota].id != undefined) {
               quotas[quota].value = math.evaluate(quotas[quota].value) * parseInt(document.getElementById('mdbmult').value);
            }
    }

    var cpu = math.evaluate(document.getElementById("instance-cores").value) * document.getElementById("instance-cores-ph").value;
    var ram = math.evaluate(document.getElementById("memory").value) * document.getElementById("memory-ph").value;
    var gpus = math.evaluate(document.getElementById("gpus").value) * document.getElementById("gpus-ph").value;
    var ssddisk = math.evaluate(document.getElementById("network-ssd-total-disk-size").value) * document.getElementById("network-ssd-total-disk-size-ph").value;
    var hdddisk = math.evaluate(document.getElementById("network-hdd-total-disk-size").value) * document.getElementById("network-hdd-total-disk-size-ph").value;
    var snap = math.evaluate(document.getElementById("total-snapshot-size").value) * document.getElementById("total-snapshot-size-ph").value;
    var cpumdb = math.evaluate(document.getElementById("mdb.cpu.count").value) * document.getElementById("mdb.cpu.count-ph").value;
    var hddmdb = math.evaluate(document.getElementById("mdb.hdd.size").value) * document.getElementById("mdb.hdd.size-ph").value;
    var ssdmdb = math.evaluate(document.getElementById("mdb.ssd.size").value) * document.getElementById("mdb.ssd.size-ph").value;
    var rammdb = math.evaluate(document.getElementById("mdb.memory.size").value) * document.getElementById("mdb.memory.size-ph").value;
    var clodst = math.evaluate(document.getElementById("total_size_quota").value) * document.getElementById("hot-st-ph").value;

    var summ =
    cpu+
    ram+
    gpus+
    ssddisk+hdddisk+
    snap+
    clodst;

    var mdbcoef = check('mdbcoef');

    console.log(mdbcoef);

    var summmdb = (cpumdb + hddmdb + ssdmdb + rammdb)/mdbcoef;

    summ+=summmdb;

    var grant = parseInt(document.getElementById("grant").value);
    var crlimit = parseInt(document.getElementById("cr-limit").value);
    var balance = parseInt(document.getElementById("balance").value);
    var computegrant = parseInt(document.getElementById("computegrant").value);
    var othergrant = parseInt(document.getElementById("othergrant").value);
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
             document.getElementById('decision').innerHTML = "Требуется пополнение";
             document.getElementById('decision').setAttribute("class", "false");
             document.getElementById('wbepaid').innerHTML = "На сумму: " + Math.abs(totalbl - Math.round(summ * 48 / curusd)) + cur +" +запас " + Math.round(summ * 2 / curusd) + cur;
        }
        document.getElementById('getcode').hidden = false;
    };
}

function getcode () {

    document.getElementById('link').hidden = false;
    document.getElementById('copy').hidden = false;
    var types= ['compute', 'mdb', 's3']
    var quotasval = {};
    var arraypath = window.location.pathname.split('/');
    try {
    quotasval['cloudId'] = document.getElementById("anyID").value;
    }
    catch (e){
    quotasval['cloudId'] = arraypath[arraypath.length - 1];
    }

    for (var type in types) {
        var quotas = document.getElementsByName(types[type]);
        quotasval[types[type]]= {}
        for (var quota in quotas) {
            if (quotas[quota].id != undefined) {
               quotasval[types[type]][quotas[quota].id] = parseInt(quotas[quota].value, 10);
            }
        }
    }
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
     var data = JSON.stringify(quotasval);
     request.send(data);

}


function copyLink() {
  document.getElementById('link').select();
  document.execCommand("copy");
}

