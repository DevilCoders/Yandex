function include (src, cont) {
	var js = document.createElement ("SCRIPT");
	js.onload = cont;
	js.src = src;
	js.type = "text/javascript"
	document.getElementsByTagName("head")[0].appendChild(js)
}
