CM.Sort = new function() {
    // tableElements is object that contains table rows and cells indexed by target/worker name; we need it to
    // get target/worker names
    this.sortIfTableUpdatedF = function(sorter, tableElements) {
        return function(update) {
            var targetOrWorkerUpdated = false;
            for (var targetOrWorkerName in update) {
                if (targetOrWorkerName in tableElements) {
                    targetOrWorkerUpdated = true;
                }
            }
            if (targetOrWorkerUpdated) {
                sorter.resortByCurrentComparer();
            }
        }
    };

    this.Sorter = function(table, enumerateF) {
        var table = table;

        var controls = {};
        var callbacks = {};
        var comparers = {};

        var table = null;
        var tableRowPredicate = function(row) {
            return 'sortValues' in row;
        };

        var stateIcons = ['&#9650;', '&#9660;', '&#9679;'];

        var lastKey = null;
        var lastState = 0;

        var indicator = null;

        this.addControl = function(key, element, fn) {
            controls[key] = element;
        };
        this.addCallback = function(key, fn) {
            callbacks[key] = fn;
        };
        this.addComparer = function(key, ascFn, descFn, customFn) {
            comparers[key] = [ascFn, descFn];
            if (customFn) comparers[key].push(customFn);
        };
        this.addComparers = function(stringFields, numberFields) {
            var compareIfUndefined = function(field, first, second) {
                var firstUndef = (typeof (first.sortValues[field]) === 'undefined');
                var secondUndef = (typeof (second.sortValues[field]) === 'undefined');
                if (firstUndef && secondUndef) {
                    return 0;
                } else if (firstUndef) {
                    return -1;
                } else if (secondUndef) {
                    return 1;
                } else {
                    return null;
                }
            };
            CM.forEach(stringFields, function(sorter, compareIfUndefined) {
                return function(field) {
                    sorter.addComparer(field, function compare(first, second) {
                        var resIfUndefined = compareIfUndefined(field, first, second);
                        if (resIfUndefined != null) {
                            return resIfUndefined;
                        } else {
                            return first.sortValues[field].localeCompare(second.sortValues[field]);
                        }
                    },
                    function (first, second) {
                        return -compare(first, second);
                    });
                };
            }(this, compareIfUndefined));
            CM.forEach(numberFields, function(sorter, compareIfUndefined) {
                return function(field) {
                    sorter.addComparer(field, function compare(first, second) {
                        var resIfUndefined = compareIfUndefined(field, first, second);
                        if (resIfUndefined != null) {
                            return resIfUndefined;
                        } else {
                            return first.sortValues[field] - second.sortValues[field];
                        }
                    },
                    function (first, second) {
                        return -compare(first, second);
                    });
                };
            }(this, compareIfUndefined));
        };
        
        this.setTable = function(t) {
            table = t;
        };
        
        var sortByComparer = function(rows, comparer) {
            var needSorting = false;
            for (var i = 1; i < rows.length; i++) {
                if (comparer(rows[i - 1], rows[i]) > 0) {
                    needSorting = true;
                    break;
                }
            }
            if (!needSorting) { // optimization: sorting of array by itself isn't expensive but dom modification is
                return;
            }

            rows.sort(comparer);
        };

        var handleCallback = function (rows, key, clear, state) {
            if (callbacks[key]) {
                CM.forEach(rows, function(r) {
                    callbacks[key](r, $(r).find("." + key), clear, state);
                });
            }
        }

        this.sort = function(key) {
            if (!(key in controls) || !(key in comparers)) {
                return;
            }

            var state = 0;
            if (key == lastKey) {
                state = (lastState + 1) % comparers[key].length;
            }

            var tableBody = CM.findTableBody(table);
            var rows = CM.collectTableRows(table, tableRowPredicate);

            handleCallback(rows, lastKey, true, lastState);
            sortByComparer(rows, comparers[key][state]);
            handleCallback(rows, key, false, state);

            CM.forEach(rows, function(r) {
                tableBody.removeChild(r);
                tableBody.appendChild(r);
            });

            if (indicator == null) {
                indicator = document.createElement('SPAN');
                indicator.setAttribute('id', 'SortingIndicator');
            } else {
                controls[lastKey].removeChild(indicator);
            }
            indicator.innerHTML = stateIcons[state];
            controls[key].insertBefore(indicator, controls[key].firstChild);

            lastKey = key;
            lastState = state;
        };
        
        this.resortByCurrentComparer = function() {
            sortByComparer(comparers[lastKey][lastState]);
        };
    };
};
