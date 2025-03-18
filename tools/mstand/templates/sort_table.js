function sortFunction(a, b, asc) {
    var result;

    /* Default ascending order */
    if (typeof asc == "undefined") asc = true;

    if (a === null) return 1;
    if (b === null) return -1;
    if (a === null && b === null) return 0;

    result = a - b;

    if (isNaN(result)) {
        return (asc) ? a.localeCompare(b) : b.localeCompare(a);
    } else {
        return (asc) ? result : -result;
    }
}

function sortTable(table, col, reverse) {
    var tb = table.tBodies[0], // use `<tbody>` to ignore `<thead>` and `<tfoot>` rows
        tr = Array.prototype.slice.call(tb.rows, 0), // put rows into array
        i;

    reverse = -((+reverse) || -1);
    tr = tr.sort(function (a, b) {
        return sortFunction(
            a.cells[col].textContent.trim(),
            b.cells[col].textContent.trim(),
            reverse > 0
        );
    });

    for (i = 0; i < tr.length; ++i) {
        tb.appendChild(tr[i]); // append each row in order
    }
}

function makeSortable(table) {
    var th = table.tHead,
        i;

    th && (th = th.rows[0]) && (th = th.cells);

    if (th) {
        i = th.length;
    } else {
        return; // if no `<thead>` then do nothing
    }

    while (--i >= 0) (function (i) {
        // closure for each column with direction
        var dir = 1;
        th[i].addEventListener('click', function () {
            sortTable(table, i, (dir = 1 - dir));
        });
    }(i));
}