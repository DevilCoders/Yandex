var addFilterListener = function(textFilter) {
    var timer = 0;
    textFilter.addEventListener('input', function() {
        var value = this.value;
        clearTimeout(timer);
        // применение фильтра через 500мс после окончания ввода
        timer = setTimeout(function() {
            filterResults(value);
        }, 500);
    });
};

var filterResults = function(rule) {
    try {
        if (filter(rule)) {
            document.querySelector('.filter__error').innerText = '';
        }
    } catch(e) {
        document.querySelector('.filter__error').innerText = e.name + ': ' + e.message;
    }
};

var isRowVisible = function(row) {
    return !row.classList.contains('metrics__row_hidden');
};

var showRow = function(row) {
    row.classList.remove('metrics__row_hidden');
};

var hideRow = function(row) {
    row.classList.add('metrics__row_hidden');
};

/**
 * Фильтрует ряды tr в таблице, оставляя только те, которые проходят условие `rule`
 * @param {String} rule - строка с plain js кодом
 * @returns {Boolean} - всегда возвращает true
 */
var filter = function(rule) {
    var rows = document.querySelectorAll('.metrics__row');
    var total = rows.length;
    var found = 0;

    rule = rule.trim();
    if (!rule) {
        found = total;
        [].forEach.call(rows, function(row) {
            showRow(row);
        });
    } else {
        console.time("Rows iterator, execution time took");
        [].forEach.call(rows, function(row) {
            if (isRuleMatch(rule, row)) {
                showRow(row);
                found++;
            } else {
                hideRow(row);
            }
        });
        console.timeEnd("Rows iterator, execution time took");

        if (document.querySelector('#cb-show-only-diff-color').checked) {
            var currentExpRows = [];
            var checkExpRows = function() {
                // проверяем, что все видимые строки не одного цвета
                var currentColor = currentExpRows[0][0];
                var sameColor = true;
                for (var j = 0; j < currentExpRows.length; ++j) {
                    if (currentColor != currentExpRows[j][0]) {
                        sameColor = false;
                        break;
                    }
                }
                if (sameColor) {
                    // скрываем все ряды одного цвета
                    for (var j = 0; j < currentExpRows.length; ++j) {
                        hideRow(currentExpRows[j][1]);
                        found--;
                    }
                }
            };

            for (var i = 0; i < rows.length; ++i) {
                if (rows[i].dataset.isFirst && currentExpRows.length > 0) {
                    checkExpRows();
                    currentExpRows = [];
                }
                if (isRowVisible(rows[i])) {
                    // все видимые строки сохраняем в массив
                    currentExpRows.push([rows[i].dataset.color, rows[i]])
                }
            }
            if (currentExpRows.length > 0) {
                checkExpRows();
            }
        }
    }

    if (found < total) {
        document.querySelector('.metrics').classList.add('metrics_filtered');
    } else {
        document.querySelector('.metrics').classList.remove('metrics_filtered');
    }

    document.querySelector('.filter__found').innerText = found + ' / ' + total;
    return true;
};

/**
 * Проверяет, выполняется ли правило `rule` для `rowNone` - ряда tr в таблице со значением метрики
 * @param {String} rule - строка с plain js кодом
 * @param {HTMLElement} rowNode — ряд tr в таблице с метрикой
 * @returns {Boolean}
 */
var isRuleMatch = function(rule, rowNode) {
    var fields = Object.keys(rowNode.dataset);
    var dataAttr;
    var value;

    // export data-attributes to local scope variables
    for (var i = 0; i < fields.length; i++) {
        dataAttr = rowNode.dataset[fields[i]];
        value = isNaN(dataAttr) ? '"' + dataAttr + '"' : +dataAttr; // convert strings to numbers
        eval(['var', fields[i], '=', value, ';'].join(' '));
    }

    return Boolean(eval(rule));
};
