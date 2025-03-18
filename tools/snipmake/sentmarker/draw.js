function drawlist (list, el) {
	var hdr = document.createElement ("H2");
	hdr.className = "dathdr";
	hdr.innerHTML = "Datasets:";
	el.appendChild (hdr);

	var ls = document.createElement ("UL");
	ls.className = "datlst";
	for (var i = 0; i < list.length; ++i) {
		var e = document.createElement ("LI");
		var a = document.createElement ("A");
		a.href = "?" + list[i];
		a.innerHTML = "dataset " + list[i];
		e.appendChild (a);
		ls.appendChild (e);
	}
	el.appendChild (ls);
}
function drawhelp (el) {
	function dr (hd) {
		var dl = document.createElement ("DL");
		var lh = document.createElement ("LH");
		lh.appendChild (document.createTextNode (hd));
		dl.appendChild (lh);
		return dl;
	}
	function ad (dl, term, def) {
		var t = document.createElement ("DT");
		t.appendChild (term);
		dl.appendChild (t);
		var t = document.createElement ("DD");
		t.appendChild (def);
		dl.appendChild (t);
	}
	function txt (s) { return document.createTextNode (s); };
	function stxt (st1, st2, s) {
		var sp1 = document.createElement ("SPAN");
		sp1.className = st1;
		var sp2 = document.createElement ("SPAN");
		sp2.className = st2;
		sp2.innerHTML = s;
		sp1.appendChild (sp2);
		return sp1;
	}
	var ls = dr ("Colors:");
	ad (ls, stxt ("sentok", "senttext", "Qwe rty."), txt ("Ok sent"));
	ad (ls, stxt ("sentbad", "senttext", "Qwe rty."), txt ("Bad sent"));
	el.appendChild (ls);

	var ls = dr ("Keys:");
	ad (ls, txt ("click"), txt ("toggle ok"));
	ad (ls, txt ("w"), txt ("step up"));
	ad (ls, txt ("s"), txt ("step down"));
	ad (ls, txt ("a"), txt ("make ok"));
	ad (ls, txt ("d"), txt ("make bad"));
	ad (ls, txt ("x"), txt ("toggle ok"));
	ad (ls, txt ("q"), txt ("scroll up"));
	ad (ls, txt ("z"), txt ("scroll down"));
	el.appendChild (ls);
}
function draw (dat, el) {
	function ss (st, inn) {
		var res = document.createElement ("SPAN");
		res.className = st;
		res.innerHTML = inn;
		return res;
	}
	var hdr = document.createElement ("h2");
	hdr.appendChild (document.createTextNode ("Request: "));
	hdr.appendChild (ss ("req", dat.userreq));
	el.appendChild (hdr);

	var hdr = document.createElement ("h2");
	hdr.appendChild (document.createTextNode ("Url: "));
	hdr.appendChild (ss ("url", dat.url));
	el.appendChild (hdr);

	var hdr = document.createElement ("h2");
	hdr.appendChild (document.createTextNode ("Title: "));
	hdr.appendChild (ss ("title", dat.title));
	el.appendChild (hdr);

	var tbl = document.createElement ("TABLE");
	tbl.className = "tbl";
	tbl.id = "tbl";
	var doc = dat.doc;
	var dat = dat.expl;
	for (var i = 0; i < dat.length; ++i) {
		var row = document.createElement ("TR");
		if (dat[i][0] == "---")
			row.className = "skip";
		else {
			row.className = "sentok";
			row.id = "sent_" + doc + "_" + i;
		}
		for (var j = 0; j < dat[i].length; ++j) {
			var cell = document.createElement ("TD");
			if (j == 0)
				cell.className = "sentnum";
			else if (j == 1)
				cell.className = "senttext";
			else
				cell.className = "sentaux";
			cell.innerHTML = dat[i][j];
			row.appendChild (cell);
		}
		tbl.appendChild (row);
	}
	el.appendChild (tbl);

	var hdr = document.createElement ("h2");
	var sv = document.createElement ("SPAN");
	sv.id = "sv";
	sv.innerHTML = "Save: ";
	hdr.appendChild (sv);
	var lnk = document.createElement ("A");
	lnk.href = "zzz";
	lnk.id = "lnk";
	lnk.innerHTML = "";
	hdr.appendChild (lnk);
	el.appendChild (hdr);

	var hdr = document.createElement ("h2");
	var ld = document.createElement ("SPAN");
	ld.id = "ld";
	ld.innerHTML = "Load: ";
	hdr.appendChild (ld);
	var lnk = document.createElement ("TEXTAREA");
	lnk.id = "ldt";
	hdr.appendChild (lnk);
	el.appendChild (hdr);

	var hdr = document.createElement ("h2");
	var lnk = document.createElement ("A");
	lnk.href = "?";
	lnk.innerHTML = "Back to the list";
	hdr.appendChild (lnk);
	el.appendChild (hdr);
}
