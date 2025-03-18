function load (x) {
	window.data = x;
}
function calcres () {
	var dat = window.data;
	var doc = dat.doc;
	dat = dat.expl;
	var res = "";
	for (var i = 0; i < dat.length; ++i) {
		if (dat[i][0] != "---") {
			res += doc + " " + i + " " + dat[i][0] + " ";
			res += window.document.getElementById ("sent_" + doc + "_" + i).className.split (' ')[0];
			res += "\n";
		}
	}
	return res;
}
function loadres (a) {
	if (a.length % 4 != 0) {
		window.alert ("ERROR: corrupt data");
		return;
	}
	var dat = window.data;
	var doc = dat.doc;
	dat = dat.expl;
	var n = 0;
	for (var i = 0; i < dat.length; ++i) {
		if (dat[i][0] != "---") {
			++n;
		}
	}
	var m = 0;
	var mdoc = 0;
	var mcrc = 0;
	for (var i = 0; i < a.length; i += 4) {
		if (a[i] == doc) {
			var j = a[i + 1];
			var k = a[i + 2];
			var r = a[i + 3];
			if (dat[j][0] == k) {
				var e = window.document.getElementById ("sent_" + doc + "_" + j);
				var c = e.className;
				e.className = r;
				if (c.split (' ').length > 1) {
					e.className = e.className + " " + c.split (' ')[1];
				}
				++m;
			} else {
				++mcrc;
			}
		} else {
			++mdoc;
		}
	}
	if (mcrc > 0) {
		window.alert ("loaded " + m + " marks (of " + n + "), seen " + mcrc + " ERRORS!");
	} else if (m == n) {
		window.alert ("loaded all " + n + " marks successfully");
	} else if (m != 0) {
		window.alert ("loaded only " + m + " marks (of " + n + ") ERROR?");
	} else if (mdoc > 0) {
		window.alert ("ERROR: marks are from different doc.");
	} else {
		window.alert ("ERROR: 0 marks loaded");
	}
}
