window.addEventListener('load', initWorkersPage, false);

function initWorkersPage() {
    var MasterElement = document.getElementById('MasterElement');
    var stateurl = MasterElement.getAttribute('stateurl');
    var urlroot = MasterElement.getAttribute('urlroot');
    var target = MasterElement.getAttribute('target');
    var timestamp = MasterElement.getAttribute('timestamp');

    Updater.register('$#', function(v) {
        if (v != timestamp)
            window.location.reload();
    });

    Updater.register('$!', function(v) {
        CMErrorNotification(v);
    });

    var update = function(e, v) {
        if (v === null) { // Filtered out
            e.row.style.display = 'none';
        } else {
            e.row.style.display = 'table-row';
            if (target) {
                CMPutShortWorkerState(e.C, v.C); // Connection
                CMPutStateLabels(e.X, v.t, urlroot + 'target/' + target + '/' + e.row.worker + '?state=%S', 'target=' + target + '&worker=' + e.row.worker + '&state=%S', v.REP, v.CronFailed); // State
                CMPutDuration(e.dA, v.dA, 'duract'); // Act duration
                CMPutDuration(e.dt, v.dt, 'durttl'); // Ttl duration
            } else {
                CMPutWorkerState(e.C, v.C); // Connection
                CMPutStateLabels(e.X, v.t, urlroot + 'worker/' + e.row.worker + '?state=%S', null, v.REP, v.CronFailed === true); // State
                CMPutStateCounts(e.T, v.T); // Targets
                CMPutDiskspace(e.D, v.D, 'diskspace'); // Diskspace
            }
            CMPutStateCounts(e.t, v.t); // Tasks
            CMPutTime(e.s, v.s, 'started'); // Last started
            CMPutTime(e.f, v.f, 'finished'); // Last finished
            CMPutTime(e.S, v.S, 'success'); // Last success
            CMPutTime(e.F, v.F, 'failure'); // Last failure
            CMPutResource(e.cM, v.cM, 'cpumax'); // Cpu max
            CMPutResource(e.ca, v.ca, 'cpuavg'); // Cpu avg
            CMPutResource(e.mM, v.mM, 'memmax'); // Mem max
            CMPutResource(e.ma, v.ma, 'memavg'); // Mem avg
            e.M.innerHTML = v.M || '&mdash;'; // Last message
        }
    }

    var elements = {};

    document.foreachElementsByName('@@', function(e) {
        elements[e.worker = e.getAttribute('worker')] = { row: e };
    });

    for (var i in set('C', 'X', 't', 'T', 's', 'f', 'S', 'F', 'dA', 'dt', 'M', 'D', 'cM', 'ca', 'mM', 'ma'))
        document.foreachElementsByName('@' + i, function(e) {
            elements[e.parentNode.worker][i] = e;
        });

    for (var i in elements)
        (function(i) {
            Updater.register(i, function(v) {
                update(elements[i], v);
            });
        })(i);

    Mouse.assignDocument(document, null);

    SmartHint.assignControls(document.getElementsByName('SH'), CMFormatCmnsmHint);

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
            number: row.getAttribute('number'),
            worker: row.getAttribute('worker'),
        };
    });

    sorter.addComparers(
        ['worker'],
        ['number', 'diskspace', 'started', 'finished', 'success', 'failure', 'duract', 'durttl', 'cpumax', 'cpuavg', 'memmax', 'memavg']
    );

    sorter.sort('number');
    Updater.registerAfter(CM.Sort.sortIfTableUpdatedF(sorter, elements));

    Updater.serv(stateurl);
}
