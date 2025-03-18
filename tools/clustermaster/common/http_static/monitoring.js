window.addEventListener('load', initMonitoringPage, false);

function initMonitoringPage() {
    var MasterElement = document.getElementById('MasterElement');
    var stateurl = MasterElement.getAttribute('stateurl');
    var timestamp = MasterElement.getAttribute('timestamp');
    var GraphObject = document.getElementById('Graph');
    var TargetsTable = document.getElementById('Targets');

    Updater.register('$#', function(v) {
        if (v != timestamp)
            window.location.reload();
    });

    var update = function(e, v) {
        if (e.t)
            CMPutStateCounts(e.t, v);
        if (e.g)
            CMPutStatePiechart(e.g, v);
    }

    var docHeight = MasterElement.scrollHeight;

    var graphDocument = GraphObject.getSVGDocument();

    var targetRows = TargetsTable.getElementsByTagName('tr');
    var rowPlug = targetRows[targetRows.length - 1];

    var tableInnerHTML = '';

    for (var maxrow = Math.floor(docHeight / rowPlug.offsetHeight), row = 0; row < maxrow; ++row) {
        tableInnerHTML += '<tr>';
        for (var maxi = maxrow * Math.ceil((targetRows.length - 1) / maxrow), i = row; i < maxi; i += maxrow)
            if (i < targetRows.length - 1)
                tableInnerHTML += targetRows[i].innerHTML;
            else
                tableInnerHTML += rowPlug.innerHTML;
        tableInnerHTML += '</tr>';
    }

    var newTargetsTable = document.createElement('table');
    newTargetsTable.setAttribute('class', 'grid');
    if (CM.getQueryString()["graphonly"])
        newTargetsTable.setAttribute('style', 'display:none');
    newTargetsTable.innerHTML = tableInnerHTML;
    TargetsTable.parentNode.replaceChild(newTargetsTable, TargetsTable);

    var elements = {};

    document.foreachElementsByName('@X', function(e) {
        var target = e.getAttribute('id');
        if (target in elements)
            elements[target].t = e;
        else
            elements[target] = { t: e };
    });

    if (graphDocument) {
        var graphRect = graphDocument.documentElement.getAttribute('viewBox').split(' ');
        GraphObject.style.width = Math.ceil(docHeight * parseFloat(graphRect[2]) / parseFloat(graphRect[3])) + 'px';

        for (var groups = graphDocument.getElementsByTagName('g'), i = 0; i < groups.length; ++i) {
            var e = groups[i];

            if (e.getAttribute('class') !== 'node')
                continue;

            var target = e.parentNode.getAttribute('id');

            e.cx = parseFloat(e.firstChild.getAttribute('cx'));
            e.cy = parseFloat(e.firstChild.getAttribute('cy'));
            e.rx = parseFloat(e.firstChild.getAttribute('rx'));
            e.ry = parseFloat(e.firstChild.getAttribute('ry'));

            if (target in elements)
                elements[target].g = e;
            else
                elements[target] = { g: e };
        }
    }

    document.documentElement.style.overflowX = 'hidden';
    document.documentElement.style.overflowY = 'hidden';

    for (var i in elements)
        (function(i) {
            Updater.register(i, function(v) {
                update(elements[i], v);
            });
        })(i);

    Updater.serv(stateurl);
}
