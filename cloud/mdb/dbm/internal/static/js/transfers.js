$(document).ready(function() {
    url = '/api/v2/transfers/';
    var table = $('#transfersTable').DataTable( {
        paging:     true,
        pageLength: 20,
        ordering:   false,
        info:       true,
        searching:  true,
        select: {
            style:    'os',
        },
        dom: 'Bfrtip',
        buttons: [
            'selectAll',
            'selectNone',
            {
                text: 'Cancel',
                action: function (e, dt, node, config) {
                    cancelTransfer();
                }
            }
        ],
        ajax: {
            url: url,
            dataSrc: ''
        },
        columns: [
            { "data": "id" },
            { "data": function ( row, type, val, meta ) {
                return '<a href="https://wall-e.yandex-team.ru/host/' + row.src_dom0 + '" target="_blank">' + row.src_dom0 + '</a>';
                }
            },
            { "data": function ( row, type, val, meta ) {
                return '<a href="https://wall-e.yandex-team.ru/host/' + row.dest_dom0 + '" target="_blank">' + row.dest_dom0 + '</a>';
                }
            },
            { "data": "container" },
            { "data": "placeholder" },
            { "data": "started" }
        ]
    } );
} );

function cancelTransfer() {
var rows = $('#transfersTable').DataTable().rows( { selected: true } ).data();
    if (rows.length == undefined || rows.length != 1) {
        $("#alertResult").html("<div class='alert alert-warning'><button " +
                               "type='button' class='close' data-dismiss=" +
                               "'alert'>&times;</button>Select 1 transfer." +
                               "</div>");
        return;
    }

    $.ajax( {
        url: '/api/v2/transfers/' + rows[0].id + '/cancel',
        method: 'POST',
        dataType: 'json',
        error: function(json) {
            var msg_type = 'danger';
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   JSON.stringify(json) + ".</div>");
        },
        success: function ( json ) {
            var msg_type = 'success';
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   "Cancelled.</div>");
            $('#transfersTable').DataTable().ajax.reload();
        }
    } );
}
