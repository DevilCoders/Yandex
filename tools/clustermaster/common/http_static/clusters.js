window.addEventListener('load', initClustersPage, false);

function initClustersPage() {
    var MasterElement = document.getElementById('MasterElement');
    var stateurl = MasterElement.getAttribute('stateurl');
    var urlroot = MasterElement.getAttribute('urlroot');
    var worker = MasterElement.getAttribute('worker');
    var target = MasterElement.getAttribute('target');
    var timestamp = MasterElement.getAttribute('timestamp');

    Updater.register('$#', function(v) {
        if (v != timestamp)
            window.location.reload();
    });

    Updater.register('$!', function(v) {
        CMErrorNotification(v);
    });

    var TasksSummary = document.getElementById('$t');
    Updater.register('$t', function(v) {
        var html = '';
        for (var i in CMTargetStatesTitles)
            html += '<tr><td>' + CMTargetStatesTitles[i] + ':</td><td>' + v[i] + '</td></tr>';
        TasksSummary.innerHTML = html;
    });

    var update = function(e, v) {
        if (v === null) { // Filtered out
            e.row.style.display = 'none';
        } else {
            e.row.style.display = 'table-row';
            CMPutStateLabels(e.X, v.t, null, 'worker=' + worker + '&target=' + target + '&task=' + e.row.task, v.REP); // State
            CMPutTime(e.s, v.s, 'started'); // Last started
            CMPutTime(e.f, v.f, 'finished'); // Last finished
            CMPutTime(e.S, v.S, 'success'); // Last success
            CMPutTime(e.F, v.F, 'failure'); // Last failure
            CMPutDuration(e.d, v.d, 'duration'); // Duration
            CMPutResource(e.cM, v.cM, 'cpumax'); // Cpu max
            CMPutResource(e.ca, v.ca, 'cpuavg'); // Cpu avg
            CMPutResource(e.mM, v.mM, 'memmax'); // Mem max
            CMPutResource(e.ma, v.ma, 'memavg'); // Mem avg
        }
    }

    var elements = {};

    document.foreachElementsByName('@@', function(e) {
        elements[e.task = e.getAttribute('task')] = { row: e };
    });

    for (var i in set('X', 's', 'f', 'S', 'F', 'd', 'cM', 'ca', 'mM', 'ma'))
        document.foreachElementsByName('@' + i, function(e) {
            elements[e.parentNode.task][i] = e;
        });

    for (var i in elements)
        (function(i) {
            Updater.register(i, function(v) {
                update(elements[i], v);
            });
        })(i);

    Mouse.assignDocument(document, null);

    SmartHint.assignControls(document.getElementsByName('SH'), CMFormatCmnsmHint);
    SmartHint.assignControls(document.getElementsByName('IH'), CMFormatTaskInfoHint);

    ActionMenu.setUrlRoot(urlroot);
    ActionMenu.assignControls(document.getElementsByName('AM'));

    var sorter = new CM.Sort.Sorter();

    document.foreachElementsByName('sortctrl', function(e) {
        var key = e.getAttribute('sortkey');
        sorter.addControl(key, e);
        e.onclick = (function(sorter, key) {
            return function() {
                sorter.sort(key);
            };
        })(sorter, key);
    });

    sorter.setTable(MasterElement);
    var rows = CM.collectTableRows(MasterElement, function(e) {
        return e.getAttribute('name') === '@@';
    });
    CM.forEach(rows, function(row) {
        row.sortValues = {
            number: row.getAttribute('task'),
            value: row.getAttribute('value'),
        };
    });

    sorter.addComparers(
        ['value'],
        ['number', 'started', 'finished', 'success', 'failure', 'duration', 'cpumax', 'cpuavg', 'memmax', 'memavg']
    );

    sorter.sort('number');
    Updater.registerAfter(CM.Sort.sortIfTableUpdatedF(sorter, elements));

    Updater.serv(stateurl);
}
