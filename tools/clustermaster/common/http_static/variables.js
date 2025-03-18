window.addEventListener('load', initVariablesPage, false);

VariablesUpdater = new BaseUpdater();

function initVariablesPage() {
    // resize table body
    (function(tbody) {
        if (tbody.rows.length > 30)
            tbody.style.height = tbody.rows.item(0).offsetHeight * 30;
    })(document.getElementById('varsList'));

    if (!Env.ReadOnly) {
        // bind controls
        var setVariableOnSelectedHosts = document.getElementById('setVariableOnSelectedHosts');
        var unsetVariableOnSelectedHosts = document.getElementById('unsetVariableOnSelectedHosts');
        var setVariable = document.getElementById('setVariable');
        var unsetVariable = document.getElementById('unsetVariable');
        var setMasterVariable = document.getElementById('setMasterVariable');
        var unsetMasterVariable = document.getElementById('unsetMasterVariable');

        setVariableOnSelectedHosts.href = "javascript:processCommand(true,true)";
        unsetVariableOnSelectedHosts.href = "javascript:processCommand(false,true)";
        setVariable.href = "javascript:processCommand(true,false)";
        unsetVariable.href = "javascript:processCommand(false,false)";
        setMasterVariable.href = "javascript:processCommand(true,'master')";
        unsetMasterVariable.href = "javascript:processCommand(false,'master')";

        // uncheck all hosts
        (function(checkboxes) {
            for (var i = 0; i < checkboxes.length; ++i)
                checkboxes[i].checked = false;
        })(document.getElementsByName('varsHost'));
    }

    var strongDefaultVars = JSON.parse(document.getElementById('strongDefaultVars').innerHTML);

    var currentState = {
        allVariables: [],
        workers: {}
    };

    currentState['strongDefaultVars'] = strongDefaultVars;

    VariablesUpdater.setHandler(function(diffFromServer) {
        var updatedCount = 0;
        for (var worker in diffFromServer.workers) updatedCount++;
        if (updatedCount > 0) {
            updateTable(currentState, diffFromServer.workers);
        }
    });
    VariablesUpdater.serv('varstatus', 0);
}

function updateTable(currentState, diffFromServerWorkers) {
    var changedWorkers = {};
    for (var changedWorker in diffFromServerWorkers) {
        changedWorkers[changedWorker] = true;
        currentState.workers[changedWorker] = diffFromServerWorkers[changedWorker];
    }

    var allVariablesTemp = {};
    for (var worker in currentState.workers) {
        var workerStatus = currentState.workers[worker].status;
        if (workerStatus != 'active') continue;
        for (var v in currentState.workers[worker].variables) {
            allVariablesTemp[v] = true;
        }
    }
    var allVariablesArrayTemp = [];
    for (var v in allVariablesTemp) allVariablesArrayTemp.push(v);
    allVariablesArrayTemp.sort();
    var variablesListWasChanged = !compareArrays(allVariablesArrayTemp, currentState.allVariables);
    currentState.allVariables = allVariablesArrayTemp;

    repaintTable(changedWorkers, variablesListWasChanged, currentState);
}

function compareArrays(ar1, ar2) {
    if (ar1.length != ar2.length) return false;
    for (var i in ar1) {
        if (ar1[i] != ar2[i]) return false;
    }
    return true;
}

function repaintTable(changedWorkers, variablesListWasChanged, currentState) {
    if (variablesListWasChanged) {
        var headRow = document.getElementById('headerRow');

        var headers = "";
        for (var i in currentState.allVariables) {
            var v = currentState.allVariables[i];

            var strongOrDefault = currentState.strongDefaultVars[v];
            if (strongOrDefault == 'strong') {
                headers += "<th class=\"disabled\">" + v + "<br><span class=\"small\">(forced)</span></th>";
            } else if (strongOrDefault == 'default') {
                headers += "<th>" + v + "<br><span class=\"small\">(has default)</span></th>";
            } else { // ordinary variable
                headers += "<th>" + v + "</th>";
            }
        }

        var th1 = headRow.getElementsByTagName('th')[0];
        var newTh1 = th1.cloneNode(true);
        headRow.innerHTML = headers;
        headRow.insertBefore(newTh1, headRow.getElementsByTagName('th')[0]);
    }

    document.foreachElementsByName('@@', function(e) {
        var worker = e.getAttribute('worker');
        if (changedWorkers[worker] || variablesListWasChanged) {
            var workerFromState = currentState.workers[worker];
            var isActive = (workerFromState.status == 'active');

            var cells = "";
            for (var i in currentState.allVariables) {
                var v = currentState.allVariables[i];
                var value = workerFromState.variables[v];
                if (!isActive) {
                    cells += "<td>unknown</td>";
                } else if (value) {
                    cells += "<td>" + value + "</td>";
                } else {
                    cells += "<td class='unset'>unset</td>";
                }
            }

            var th = e.getElementsByTagName('th')[0];
            var newTh = th.cloneNode(true);

            var labels = newTh.getElementsByTagName("label");
            if (labels.length > 0) { // there is no label element for r/o mode
                var input = labels[0].getElementsByTagName("input")[0];
                input.disabled = !isActive;
            }

            e.innerHTML = cells;
            e.insertBefore(newTh, e.getElementsByTagName('td')[0]);
        }
    });
}

function processCommand(doSet, useSelection) {
    var urlroot = document.getElementById('MasterElement').getAttribute('urlroot');

    var name, value, workers;

    var summary = doSet ? 'Setting variable' : 'Unsetting variable';

    if (useSelection && useSelection !== "master") {
        workers = [];

        var checkboxes = document.getElementsByName('varsHost');

        for (var i = 0; i < checkboxes.length; ++i) {
            if (checkboxes[i].checked)
                workers.push(checkboxes[i].value);
        }

        if (workers.length === 0) {
            alert('No hosts selected.');
            return;
        }
    }

    if (useSelection === "master") {
        summary += ' on master.';
    } else if (!workers) {
        summary += ' on all hosts.';
    } else if (workers.length == 1) {
        summary += ' on ' + workers[0] + '.';
    } else {
        summary += ' on ' + workers.length + ' hosts:\n\n';
        summary += workers.slice(0, 24).toString().replace(/,/g, ', ') + (workers.length > 24 ? ', ...' : '');
    }

    while (true) {
        name = prompt(summary + '\n\nVariable name:');
        if (name === null)
            return;
        if (/^[_a-z][_a-z\.\d]*$/i.test(name))
            break;
        alert('Incorrect variable name.');
    }

    if (doSet) {
        value = prompt(summary + '\n\n"' + name + '" value:');
        if (value === null)
            return;
    } else {
        if (!confirm(summary + '\n\nUnset "' + name + '"?'))
            return;
    }

    var commandUrl = urlroot + 'variables/' + (doSet ? 'set' : 'unset') + '?name=' + escape(name);

    if (value)
        commandUrl += '&value=' + escape(value);

    if (useSelection === "master")
        commandUrl += '&master=';

    if (!workers) {
        var request = new XMLHttpRequest();
        //reload on master variables
        request.addEventListener("loadend", function () {
            if (useSelection === "master")
                location.reload();
        });

        request.open('GET', commandUrl, true);
        request.send(null);
    } else {
        for (var i = 0; i < workers.length; ++i) {
            var request = new XMLHttpRequest();

            request.open('GET', commandUrl + '&worker=' + escape(workers[i]), true);
            request.send(null);
        }
    }
}
