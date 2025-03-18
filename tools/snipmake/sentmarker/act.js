function makecur (el) {
	if (actions.prevsent)
		actions.prevsent.className = actions.prevsent.className.split (' ')[0];
	el.className = el.className.split (' ')[0] + ' ' + "sentcur";
	actions.prevsent = el;
}
function unsave () {
	document.getElementById ("lnk").innerHTML = "";
}
function actions (dat) {
	prevsent = null;
	function makeok (el) { el.className = "sentok"; makecur (el); unsave (); }
	function makebad (el) { el.className = "sentbad"; makecur (el); unsave (); }
	function isbad (el) { return el.className.split (' ')[0] == "sentbad"; }
	function isskip (el) { return el.className.split (' ')[0] == "skip"; }
	function toggleok (el) { isbad (el) ? makeok (el) : makebad (el); }
	function clc () { toggleok (this); }
	function getkey (ev) {
		return String.fromCharCode (ev.charCode ? ev.charCode : ev.keyCode);
	}
	function nxt (el) { return el.nextSibling ? el.nextSibling : el; }
	function prv (el) { return el.previousSibling ? el.previousSibling : el; }
	function kp (e) {
		if (!actions.prevsent)
			return;
		var c = getkey (e);
		if (c == "q")
			window.scrollBy (0, -actions.prevsent.clientHeight);
		if (c == "z")
			window.scrollBy (0, actions.prevsent.clientHeight);
		if (c == "w")
			makecur (prv (actions.prevsent));
		if (c == 's')
			makecur (nxt (actions.prevsent));
		if (isskip (actions.prevsent))
			return;
		if (c == "d")
			makebad (actions.prevsent);
		if (c == "a")
			makeok (actions.prevsent);
		if (c == "x")
			toggleok (actions.prevsent);
	}
	var doc = dat.doc;
	var dat = dat.expl;
	for (var i = 0; i < dat.length; ++i)
	if (dat[i][0] != "---") {
		var el = document.getElementById ("sent_" + doc + "_" + i);
		el.addEventListener ("click", clc, false);
	}
	document.onkeypress = kp;

	function clcsv () {
		var e = document.getElementById ("lnk");
		e.href = "data:text/plain;base64," + Base64.encode (calcres ());
		e.innerHTML = "Result";
	}
	document.getElementById ("sv").addEventListener ("click", clcsv, false);

	function clcld () {
		var e = document.getElementById ("ldt");
		var s = e.value.replace (/^\s+|\s+$/g, '').replace (/\s+/g, ' ');
		if (s.length == 0) {
			return;
		}
		var a = s.split (' ');
		if (a.length == 1) {
			s = Base64.decode (s);
			a = s.split (' ');
		}
		loadres (a);
	}
	document.getElementById ("ld").addEventListener ("click", clcld, false);
}
