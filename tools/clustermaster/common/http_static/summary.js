window.addEventListener('load', initSummaryPage, false);

function initSummaryPage() {
    var MasterElement = document.getElementById('MasterElement');
    var stateurl = MasterElement.getAttribute('stateurl');
    var timestamp = MasterElement.getAttribute('timestamp');

    Updater.register('$#', function(v) {
        if (v != timestamp)
            window.location.reload();
    });

    Updater.register('$!', function(v) {
        CMErrorNotification(v);
    });

    var WorkersTable = document.getElementById('$W');
    Updater.register('$W', function(v) {
        var html = '';

        for (var i in CMWorkerStatesTitles)
            html += '<tr><td>' + CMWorkerStatesTitles[i] + ':</td><td>' + v[i] + '</td></tr>';

        WorkersTable.innerHTML = html;
    });

    var TargetsTable = document.getElementById('$T');
    Updater.register('$T', function(v) {
        var html = '';

        for (var i in CMTargetStatesTitles)
            html += '<tr><td>' + CMTargetStatesTitles[i] + ':</td><td>' + v[i] + '</td></tr>';

        TargetsTable.innerHTML = html;
    });

    var TasksTable = document.getElementById('$t');
    Updater.register('$t', function(v) {
        var html = '';

        for (var i in CMTargetStatesTitles)
            html += '<tr><td>' + CMTargetStatesTitles[i] + ':</td><td>' + v[i] + '</td></tr>';

        TasksTable.innerHTML = html;
    });

    Updater.serv(stateurl);
}
