$(document).ready(function() {
    url = '/api/v2/volumes/';
    query_string = decodeURIComponent(window.location.hash).replace('#', '');
    $('#inputQueryString').val(query_string);

    if (query_string != undefined && query_string != '') {
        url = '/api/v2/volumes/?query=' + query_string;
    }
    var table = $('#volumesTable').DataTable( {
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
                text: 'Edit',
                action: function (e, dt, node, config) {
                    showVolumesEditModal();
                }
            }
            ],
        ajax: {
            url: url,
            dataSrc: ''
            },
        columns: [
            { "data": function ( row, type, val, meta ) {
                return '<a href="https://wall-e.yandex-team.ru/host/' + row.dom0 + '" target="_blank">' + row.dom0 + '</a>';
                }
            },
            { "data": "dom0_path" },
            { "data": function ( row, type, val, meta ) {
                return '<a href="/containers#fqdn%3D' + row.container + '" target="_blank">' + row.container + '</a>';
                }
            },
            { "data": "path" },
            { "data": function ( row, type, val, meta ) {
                return format_limits(row.space_guarantee, row.space_limit);
                }
            },
            { "data": function ( row, type, val, meta ) {
                return format_limits(row.inode_guarantee, row.inode_limit, false);
                }
            },
            { "data": function ( row, type, val, meta ) {
                return '<span class="label label-info">' + row.backend + '</span>';
                }
            },
            { "data": function ( row, type, val, meta ) {
                if (row.read_only == true) {
                    return '<span class="label label-danger">NO</span>';
                } else {
                    return '<span class="label label-success">YES</span>';
                    }
                }
            }
        ]
    } );


    window.onhashchange = function() {
        url = '/api/v2/volumes/';
        query_string = decodeURIComponent(window.location.hash).replace('#', '');
        $('#inputQueryString').val(query_string);
        if (query_string != undefined && query_string != '') {
            url = '/api/v2/volumes/?query=' + query_string;
        }
        $('#volumesTable').DataTable().ajax.url(url).load();
    }

} );


$("#showVolumes").on('click', function(evt) {
    evt.preventDefault();
    url = '/api/v2/volumes/';
    query_string = $('#inputQueryString').val();
    if (query_string != undefined && query_string != '') {
        url = '/api/v2/volumes/?query=' + query_string; }

    window.location.hash = encodeURIComponent(query_string);
    $('#volumesTable').DataTable().ajax.url(url).load();
});


$("#btnSaveVolumes").on('click', function(evt) {
    evt.preventDefault();
    var rows = $('#volumesTable').DataTable().rows( { selected: true } ).data();
    var container = rows[0].container;
    console.log(container);

    var changed = {};
    $.each( mapping, function( key, v ) {
        console.log(key);
        db_key = mapping[key]['name'];
        var value = document.getElementById(key).value;
        if (value != undefined && value != '') {
            changed[db_key] = value;
        }
    });

    var current_ro_value = $('#inputReadOnly label.active input').val();
    if (current_ro_value == 'true') {
        var ro = true;
    } else {
        var ro = false;
    }
    if (rows[0].read_only != ro) {
        changed['read_only'] = ro;
    }

    console.log(changed);
    if ($.isEmptyObject(changed)) {
        var msg_type = 'warning';
        var msg = 'Nothing has been changed';
        $("#alertResult").html("<div class='alert alert-" + msg_type +
                               "'><button type='button' class='close' " +
                               "data-dismiss='alert'>&times;</button>" +
                               msg + ".</div>");
        $('#modalEditVolumes').modal('hide');
        return;
    }

    $.ajax( {
        url: '/api/v2/volumes/' + container + '?path=' + rows[0].path,
        method: 'POST',
        dataType: 'json',
        contentType: "application/json; charset=utf-8",
        data: JSON.stringify(changed),
        error: function(json) {
            console.log(json);
            var msg_type = 'danger';
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   JSON.stringify(json['responseJSON']) + ".</div>");
            $('#modalEditVolumes').modal('hide');
        },
        success: function ( json ) {
            console.log(json);
            var msg_type = 'success';
            var msg = format_deploy(json);
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   msg + ".</div>");
            if (json['warning'] != undefined) {
                $("#alertResult").append(
                        "<div class='alert alert-warning'><button type='button'" +
                        " class='close' data-dismiss='alert'>&times;</button>" +
                        json['warning'] + '</div>');
            }
            $('#modalEditVolumes').modal('hide');
            $('#volumesTable').DataTable().ajax.reload();
        }
    } );
});


var mapping = {
    'inputDom0Path': {
        'name': 'dom0_path',
    },
    'inputPath': {
        'name': 'path',
    },
    'inputSpaceGuarantee': {
        'name': 'space_guarantee',
        'format': true
    },
    'inputSpaceLimit': {
        'name': 'space_limit',
        'format': true
    },
    'inputInodeGuarantee': {
        'name': 'inode_guarantee'
    },
    'inputInodeLimit': {
        'name': 'inode_limit',
        'format': true
    },
    'inputBackend': {
        'name': 'backend'
    },
}


function showVolumesEditModal() {
    var rows = $('#volumesTable').DataTable().rows( { selected: true } ).data();
    if (rows.length == undefined || rows.length != 1) {
        $("#alertResult").html("<div class='alert alert-warning'><button " +
                               "type='button' class='close' data-dismiss=" +
                               "'alert'>&times;</button>Select 1 volume." +
                               "</div>");
        return;
    }
    $('#modalEditVolumesSmall').html('Editing ' + rows[0].container);

    $.each( mapping, function( key, value ) {
        if (mapping[key]['format']) {
            var v = format_bytes(rows[0][value.name]);
        } else {
            var v = rows[0][value.name];
        }
        document.getElementById(key).placeholder = v;
        document.getElementById(key).value = '';
    });

    if (rows[0].read_only) {
        $('#inputVolumeRW').removeClass('active');
        $('#inputVolumeRO').addClass('active');
    } else {
        $('#inputVolumeRW').addClass('active');
        $('#inputVolumeRO').removeClass('active');
    }

    $('#modalEditVolumes').modal('show');
}
