var system = require('system');
console.error = function () {
    system.stderr.write(Array.prototype.join.call(arguments, ' ') + '\n');
};

function dopage(html, onend) {
	var page = new WebPage();

	var screenw = 1034;
	var screenh = 768;
	page.viewportSize = { width: screenw, height: screenh };

	function insidepage() {
		//html text nodes somewhy don't have getBoundingClientRect&stuff. So for <span>a<b>c</b>d</span> "a" and "d" don't have usefull info, wrap them in <span class=hackd>
		function hack(n) {
			if (n.tagName == "SCRIPT" || n.tagName == "PRE" || n.tagName == "STYLE") {
				return;
			}
			for (var i = 0; i < n.childNodes.length; ++i) {
				hack(n.childNodes[i])
				if (n.childNodes[i].nodeType == 3 && n.childNodes[i].innerText != "") {
					var nc = document.createElement("SPAN");
					nc.className = "hackd"
					nc.appendChild (n.childNodes[i].cloneNode())
					n.replaceChild (nc, n.childNodes[i])
				}
			}
		}
		hack (document.documentElement)
		function isempty(n) {
			var fc = n.getBoundingClientRect
			if (fc != undefined && fc != null) {
				var c = n.getBoundingClientRect();
				if (c != undefined && c != null) {
					if (c.width != 0 && c.height != 0) {
						return false;
					}
				}
			}
			return true;
		}
		function isvisible(n) {
			if (isempty(n)) {
				return false;
			}
			var s = getComputedStyle (n, null);
			if (s != null && s.display === 'none') {
				return false;
			}
			return true;
		}
		function go(n) {
			if (!isvisible(n)) {
				return null;
			}
			var res = {
				"class" : n.className,
				"href" : n.href,
				"src" : n.src,
				"tag" : n.tagName,
				"innerText" : n.innerText,
				"bounds" : n.getBoundingClientRect ? n.getBoundingClientRect() : undefined,
				"name" : n.name,
				"value" : n.value,
				"domChildren" : []
			};
			for (var i = 0; i < n.childNodes.length; ++i) {
				var x = go (n.childNodes[i]);
				if (x != null) {
					res["domChildren"].push (x)
				}
			}
			if (res["domChildren"].length == 0) {
				delete res["domChildren"]
			}

			return res;
		}
		var res = go (document.documentElement)
		return res
	}

	function onopen() {
		var s = page.renderBase64("png");
		var c = page.content;
		var r = page.evaluate(insidepage)
		var res = {
			"layout" : r,
			"pngImage" : s,
		}
		var sres = JSON.stringify(res, undefined, 2)
		var s2 = page.renderBase64("png");
		var c2 = page.content
		if (s != s2) {
			var msg = "";
			msg += "something went wrong\n";
			msg += "<img src='data:image/png;base64," + s + "' />\n";
			msg += "<img src='data:image/png;base64," + s2 + "' />\n";
			msg += c + "\n";
			msg += c2 + "\n";
			onend(msg, sres);
		} else {
			onend(undefined, sres);
		}
	}

	function onmaybeopen(status) {
		if (status !== 'success') {
			onend('Unable to open html\n', undefined);
		} else {
			setTimeout(onopen, 1000);
		}
	}

	page.onLoadFinished = onmaybeopen
	page.setContent(html, "http://www.hamster.yandex.ru/search/?text=imnotaquery")
}

function dostdin() {
	var s = system.stdin.read();

	dopage(s, function(err, res) {
		if (err != undefined) {
			console.error(err)
		}
		if (res != undefined) {
			console.log(res)
		}
		phantom.exit()
	})
}

dostdin();
