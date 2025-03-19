$(document).ready(function() {
    url = '/api/v2/volume_backups/';
    query_string = decodeURIComponent(window.location.hash).replace('#', '');
    $('#inputQueryString').val(query_string);

    if (query_string != undefined && query_string != '') {
        url = url + '?query=' + query_string;
    }
    var table = $('#volumeBackupsTable').DataTable( {
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
                text: 'Delete',
                action: function (e, dt, node, config) {
                    deleteVolumeBackup();
                }
            }
        ],
        ajax: {
            url: url,
            dataSrc: ''
        },
        columns: [
            { "data": "container" },
            { "data": "path" },
            { "data": function ( row, type, val, meta ) {
                return '<a href="https://wall-e.yandex-team.ru/host/' + row.dom0 + '" target="_blank">' + row.dom0 + '</a>';
                }
            },
            { "data": "dom0_path" },
            { "data": "create_ts" },
            { "data": "space_limit" },
            { "data": "disk_id" }
        ]
    } );
} );

$("#showVolumeBackups").on('click', function(evt) {
    evt.preventDefault();
    url = '/api/v2/volume_backups/';
    query_string = $('#inputQueryString').val();
    if (query_string != undefined && query_string != '') {
        url = url + '?query=' + query_string;
    }

    window.location.hash = encodeURIComponent(query_string);
    $('#volumeBackupsTable').DataTable().ajax.url(url).load();
});

function deleteVolumeBackup() {
var rows = $('#volumeBackupsTable').DataTable().rows( { selected: true } ).data();
    if (rows.length == undefined || rows.length != 1) {
        $("#alertResult").html("<div class='alert alert-warning'><button " +
                               "type='button' class='close' data-dismiss=" +
                               "'alert'>&times;</button>Select 1 volume backup." +
                               "</div>");
        return;
    }

    $.ajax( {
        url: '/api/v2/volume_backups/' + rows[0].dom0 + '/' + rows[0].container + '?path=' + rows[0].path,
        method: 'DELETE',
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
            var msg = format_deploy(json);
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   msg + ".</div>");
            $('#volumeBackupsTable').DataTable().ajax.reload();
        }
    } );
}
