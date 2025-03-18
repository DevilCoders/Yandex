# encoding: utf-8

html = ur"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8"/>
    <title>Бок о бок</title>
$$styles$$
$$scripts$$
</head>
$$body$$
</html>
"""
styles = ur"""    <style type="text/css">
        table.job {
            margin: 0 auto;
        }
        hr.st1 {
            border: 0;
            height: 1px;
            background: #333;
            background-image: -webkit-linear-gradient(left, #ccc, #333, #ccc);
            background-image:    -moz-linear-gradient(left, #ccc, #333, #ccc);
            background-image:     -ms-linear-gradient(left, #ccc, #333, #ccc);
            background-image:      -o-linear-gradient(left, #ccc, #333, #ccc);
        }
        td.head {
            font-size: 16px;
            font-weight: bold;
            width: 10px;
        }
        td.query {
            text-align: center;
            font-size: 16px;
            padding: 5px;
        }
        td.region {
            text-align: center;
            font-size: 16px;
            padding: 5px;
            font-weight: bold;
        }
        div.snip {
            width: 610px;
            min-height: 120px;
            padding: 15px;
        }
        div.title {
            font-size: 16px;
        }
        body {
            font: .8em Arial,Helvetica,sans-serif;
            color: black;
        }
        a.link {
            color: #00C;
        }
        div.green {
            color: #060;
        }
        td.mark {
            font-size: 16px;
            text-align: center;
        }
        td.mark-title {
            font-weight: bold;
            font-size: 16px;
            text-align: center;
            padding-top: 20px;
        }
        span.mark {
            padding-right: 20px;
            padding-left: 20px;
        }
        td.buttons {
            text-align: center;
        }
        input.button {
            font-size: 16px;
        }

        div.result {
            font-size: 16px;
            margin: 0 auto;
            width: 625px;
            display: none;
        }
    </style>"""

scripts = ur"""
    <script type="text/javascript">
        var fileName="$$fileName$$"
        var pairs = [
$$pairs$$
        ]
        var startTime = (new Date()).valueOf();
        setTimeout(timer, 0);
        var currentPair = 0
        var pairCount = pairs.length
        var names=["inf", "cont", "read"];

        function timer() {
            var elapsed = (new Date()).valueOf() - startTime;
            var delay = 1001 - elapsed%1000;
            var el = document.getElementById("timer");
            el.innerHTML = timeToStr(elapsed)
            setTimeout(timer, delay);
        }

        function timeToStr(tm) {
            tm = Math.floor(tm/1000);
            var seconds = tm % 60;
            tm = Math.floor(tm/60);
            var minutes = tm;
            return minutes + ":" + format(seconds, 2);
        }

        function format(d, len) {
            var str = d.toString();
            while (str.length < len)
                str = "0" + str;
            return str;
        }

        function renderSnip(id, snip) {
            var el = document.getElementById(id)
            var lnk = '<div class="title"><a class="link" target="_blank" href="' + snip.url + '">' +  snip.title + '</a></div>';
            var body = '<div class="body">' + snip.snip + '</div>';
            var green = '<div class="green">' + snip.green + '</div>';
            el.innerHTML = lnk + body + green;
        }

        function renderPair(pair) {
            var progress = document.getElementById("progress");
            progress.innerHTML = (currentPair + 1) + "&nbsp;из&nbsp;" + pairCount;
            var query = document.getElementById("queryId");
            query.href = "http://yandex.ru/yandsearch?lr=" + pair.regId + "&text=" + encodeURIComponent(pair.query) + "&site=" + encodeURIComponent(pair.left.url);
            query.innerHTML = pair.query;
            var region = document.getElementById("regId");
            region.innerHTML = pair.region;
            renderSnip("leftSnip", pair.left);
            renderSnip("rightSnip", pair.right);
            var reg = ""
            for (var i = 0; i < names.length; ++i) {
                for (var j = 0; j < 3; ++j) {
                    var id = names[i] + "-" + j;
                    var radio = document.getElementById(id);
                    radio.checked = (pair.marks[i] == j);
                }
            }
        }

        function prev() {
            if (currentPair == 0)
                return;
            var job = document.getElementById("job");
            job.style.display = "";
            var resDiv = document.getElementById("result");
            resDiv.style.display = "none";
            --currentPair;
            renderPair(pairs[currentPair]);
        }

        function getAnswer(pair) {
            var res = Array();
            res.push(pair.query);
            res.push(pair.left.url);
            res.push(pair.regId);
            res.push(pair.left.title);
            res.push(pair.left.snip);
            res.push(pair.right.title);
            res.push(pair.right.snip);
            var marks = ["left", "eq", "right"];
            for (var i = 0; i < names.length; ++i) {
                res.push(names[i]+":"+marks[pair.marks[i]]);
            }
            return res.join("\t") + "\n";
        }

        function renderAnswer() {
            var job = document.getElementById("job");
            job.style.display = "none";
            var resDiv = document.getElementById("result");
            resDiv.style.display = "block";
            var resArea = document.getElementById("resultArea");
            var elapsed = (new Date()).valueOf() - startTime;
            var res = fileName + "\t" + Math.floor(elapsed/1000) + "\n";
            for (var i = 0; i < pairs.length; ++i) {
                res += getAnswer(pairs[i]);
            }
            resArea.value = res;
            alert("Пожалуйста сохраните результаты в текстовом файле!");
            resArea.select();
        }

        function next() {
            var pair = pairs[currentPair]
            for (var i = 0; i < pair.marks.length; ++i) {
                if (pair.marks[i] == -1) {
                    alert("Пожалуйста, оцените все критерии!");
                    return false;
                }
            }
            ++currentPair;
            if (currentPair < pairCount) {
                renderPair(pairs[currentPair]);
                return true;
            }
            else {
                renderAnswer();
            }
            return true;
        }

        function mark(markNum, ansNum) {
            pairs[currentPair].marks[markNum] = ansNum;
            var id = names[markNum] + "-" + ansNum;
            radio = document.getElementById(id);
            radio.checked = true;
        }
    </script>"""

body = ur"""<body onload="renderPair(pairs[0])">
<table class="job" id="job">
<tr>
<td id="progress" class="head"></td><td><hr class="st1"/></td><td id="timer" class="head"></td>
</tr>
<tr>
<td colspan="3" class="query"><a class="link" target="_blank" id="queryId" href=""></a></td>
</tr>
<tr>
<td colspan="3" class="region" id="regId"></td>
</tr>
<tr>
<td colspan="3">
<table>
<tr><td>
<div class="snip" id="leftSnip">
</div>
</td><td>
<div class="snip" id="rightSnip">
</div>
</td></tr></table>
</td>
</tr>
$$marks$$
<tr><td></td><td><hr class="st1"/></td><td></td></tr>
<tr>
<td colspan="3" class="buttons">
<input class="button" type="button" value="Назад" onclick="prev()">
<input class="button" type="button" value="Далее" onclick="next()">
</td>
</tr>
</table>

<div class="result" id="result">
<b>Результат:</b><br/>
<textarea id="resultArea" cols="75" rows="10" readonly>
</textarea>
<input class="button" type="button" value="Назад" onclick="prev()">
</div>

</body>"""

mark = ur"""<tr>
<td colspan="3" class="mark-title">$$fullname$$</td>
</tr>
<tr>
<td colspan="3" class="mark">
<span class="mark" onclick="mark($$number$$, 0)"> <input type="radio" name="$$name$$" id="$$name$$-0"> Левый </span>
<span class="mark" onclick="mark($$number$$, 1)"> <input type="radio" name="$$name$$" id="$$name$$-1"> Одинаково </span>
<span class="mark" onclick="mark($$number$$, 2)"> <input type="radio" name="$$name$$" id="$$name$$-2"> Правый </span>
</td>
</tr>"""
