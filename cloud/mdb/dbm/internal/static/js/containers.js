$(document).ready(function() {
    url = '/api/v2/containers/';
    query_string = decodeURIComponent(window.location.hash).replace('#', '');
    $('#inputQueryString').val(query_string);

    if (query_string != undefined && query_string != '') {
        url = '/api/v2/containers/?query=' + query_string;
    }
    var table = $('#containersTable').DataTable( {
        paging:     true,
        pageLength: 20,
        ordering:   false,
        info:       true,
        searching:  true,
        select: {
            style:    'os',
            selector: 'td:not(:first-child)'
        },
        dom: 'Bfrtip',
        buttons: [
            'selectAll',
            'selectNone',
            {
                text: 'Edit',
                action: function (e, dt, node, config) {
                    showContainersEditModal();
                }
            },
            {
                text: 'Launch',
                action: function (e, dt, node, config) {
                    showContainersLaunchModal();
                }
            },
            {
                text: 'Delete',
                action: function (e, dt, node, config) {
                    showContainerDeleteModal();
                }
            }
            ],
        ajax: {
            url: url,
            dataSrc: ''
            },
        columns: [
            {
                "className":      'details-control',
                "orderable":      false,
                "data":           null,
                "defaultContent": ""
            },
            { "data": function ( row, type, val, meta ) {
                return '<a href="https://wall-e.yandex-team.ru/host/' + row.dom0 + '" target="_blank">' + row.dom0 + '</a>';
                }
            },
            { "data": "fqdn" },
            { "data": function ( row, type, val, meta ) {
                if (!row.cluster_name.match("^" + row.fqdn.substring(0,4))) {
                    return '<a href="https://yasm.yandex-team.ru/template/panel/mdbdom0/hosts=' +
                       row.dom0 + ';ctype=' + row.cluster_name + '/" target="_blank">' + row.cluster_name + '</a>';
                }
                prj = row.fqdn.split('.')[0].slice(0, -1);
                return '<a href="https://yasm.yandex-team.ru/template/panel/mdbdom0/hosts=' +
                   row.dom0 + ';prj=' + prj + '/" target="_blank">' + row.cluster_name + '</a>';
                }
            },
            { "data": function ( row, type, val, meta ) {
                return format_limits(row.cpu_guarantee, row.cpu_limit, false);
                }
            },
            { "data": function ( row, type, val, meta ) {
                return format_limits(row.memory_guarantee, row.memory_limit);
                }
            },
            { "data": function ( row, type, val, meta ) {
                return format_limits(row.net_guarantee, row.net_limit);
                }
            },
            { "data": function ( row, type, val, meta ) {
                return format_limits(undefined, row.io_limit);
                }
            }
        ]
    } );


    $('#containersTable tbody').on('click', 'td.details-control', function () {
        var tr = $(this).closest('tr');
        var row = table.row( tr );

        if ( row.child.isShown() ) {
            // This row is already open - close it
            row.child.hide();
            tr.removeClass('shown');
        }
        else {
            // Open this row
            row.child( format_volume(row.data()) ).show();
            tr.addClass('shown');
        }

    } );


    window.onhashchange = function() {
        url = '/api/v2/containers/';
        query_string = decodeURIComponent(window.location.hash).replace('#', '');
        $('#inputQueryString').val(query_string);
        if (query_string != undefined && query_string != '') {
            url = '/api/v2/containers/?query=' + query_string;
        }
        $('#containersTable').DataTable().ajax.url(url).load();
    }

} );


$("#showContainers").on('click', function(evt) {
    evt.preventDefault();
    url = '/api/v2/containers/';
    query_string = $('#inputQueryString').val();
    if (query_string != undefined && query_string != '') {
        url = '/api/v2/containers/?query=' + query_string; }

    window.location.hash = encodeURIComponent(query_string);
    $('#containersTable').DataTable().ajax.url(url).load();
});


$("#btnSaveContainers").on('click', function(evt) {
    evt.preventDefault();
    var rows = $('#containersTable').DataTable().rows( { selected: true } ).data();
    var fqdn = rows[0].fqdn

    var changed = {};
    $.each( mapping, function( key, v ) {
        db_key = mapping[key]['name'];
        var value = document.getElementById(key).value;
        if (value != undefined && value != '') {
            changed[db_key] = value;
        }
    });
    if ($.isEmptyObject(changed)) {
        var msg_type = 'warning';
        var msg = 'Nothing has been changed';
        $("#alertResult").html("<div class='alert alert-" + msg_type +
                               "'><button type='button' class='close' " +
                               "data-dismiss='alert'>&times;</button>" +
                               msg + ".</div>");
        $('#modalEditContainers').modal('hide');
        return;
    }

    $.ajax( {
        url: '/api/v2/containers/' + fqdn,
        method: 'POST',
        dataType: 'json',
        contentType: "application/json; charset=utf-8",
        data: JSON.stringify(changed),
        error: function(json) {
            var msg_type = 'danger';
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   JSON.stringify(json['responseJSON']) + ".</div>");
            $('#modalEditContainers').modal('hide');
        },
        success: function ( json ) {
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
            $('#modalEditContainers').modal('hide');
            $('#containersTable').DataTable().ajax.reload();
        }
    } );
});


$("#inputPlacement :input").change(function() {
    var placement = $('#inputPlacement label.active input').val();
    if (placement == 'manual') {
        $('#InputDom0Form').collapse('show');
    } else {
        $('#InputDom0Form').collapse('hide');
    }
});


$("#inputFQDN").on('keyup', function(evt)  { input_fqdn(); });
$("#inputFQDN").on('change', function(evt) { input_fqdn(); });


$("#inputDom0").on('keyup', function(evt) {
    var dom0 = $('#inputDom0').val();
    if (dom0 != '') {
        $('#inputDom0').parent().parent().removeClass('has-error');
    }
});


$("#selectProject").on('change', function() {
    var selected = $(this).find("option:selected").val();
    if (selected == 'sandbox') {
        $('#inputCpuGuaranteeLaunch').val(1);
        $('#inputCpuLimitLaunch').val(2);
        $('#inputMemoryGuaranteeLaunch').val('1 GB');
        $('#inputMemoryLimitLaunch').val('4 GB');
        $('#inputNetGuaranteeLaunch').val('2 MB');
        $('#inputNetLimitLaunch').val('8 MB');
        $('#inputIOLimitLaunch').val('10 MB');
        $('#inputSpaceLimit').val('50 GB');
        $('#inputProjectIdLaunch').val('0:47b0');
        $('#inputManagingProjectIdLaunch').val('0:1589');
    } else if (selected == 'pers' || selected == 'pgaas') {
        $('#inputCpuGuaranteeLaunch').val(1);
        $('#inputCpuLimitLaunch').val(4);
        $('#inputMemoryGuaranteeLaunch').val('4 GB');
        $('#inputMemoryLimitLaunch').val('16 GB');
        $('#inputNetGuaranteeLaunch').val('16 MB');
        $('#inputNetLimitLaunch').val('32 MB');
        $('#inputIOLimitLaunch').val('100 MB');
        $('#inputSpaceLimit').val('100 GB');
        $('#inputManagingProjectIdLaunch').val('0:1589');
        if (selected == 'pgaas') {
            $('#inputProjectIdLaunch').val('0:1589');
        } else {
            $('#inputProjectIdLaunch').val('0:640');
        }
    }
    $('#selectProject').parent().parent().parent().removeClass('has-error');
});


$("#btnCheckLaunchConfig").on('click', function(evt) {
    evt.preventDefault();
    var has_errors = false;

    var project = $("#selectProject").find("option:selected").val();
    if (project == undefined || project == '' ) {
        $('#selectProject').parent().parent().parent().addClass('has-error');
        has_errors = true;
    }

    var fqdn = $("#inputFQDN").val();
    if (fqdn == undefined || fqdn == '' ) {
        $('#inputFQDN').parent().parent().addClass('has-error');
        has_errors = true;
    }

    var placement = $('#inputPlacement label.active input').val();
    if (placement == 'manual') {
        var dom0 = $("#inputDom0").val();
        if (dom0 == undefined || dom0 == '' ) {
            $('#inputDom0').parent().parent().addClass('has-error');
            has_errors = true;
        }
    }

    var dc_letter = fqdn.split('.')[0].slice(-1);
    if (dc_letter == 'e') {
        var dc = 'iva';
    } else if (dc_letter == 'f') {
        var dc = 'myt';
    } else if (dc_letter == 'h') {
        var dc = 'sas';
    } else if (dc_letter == 'i') {
        var dc = 'man';
    } else if (dc_letter == 'k') {
        var dc = 'vla';
    }

    if (has_errors) {
        return;
    }

    var volume = {
        'path': '/',
        'dom0_path': $("#inputDom0Path").val(),
        'backend': $("#selectVolumeBackend").find("option:selected").val()
    }
    var read_only = $('#inputReadOnly label.active input').val();
    if (read_only == 'true') {
        volume.read_only = true;
    }
    var space_guarantee = $("#inputSpaceGuarantee").val();
    if (space_guarantee != '') {
        volume.space_guarantee = space_guarantee;
    }
    var space_limit = $("#inputSpaceLimit").val();
    if (space_limit != '') {
        volume.space_limit = space_limit;
    }
    var inode_guarantee = $("#inputInodeGuarantee").val();
    if (inode_guarantee != '') {
        volume.inode_guarantee = inode_guarantee;
    }
    var inode_limit = $("#inputInodeLimit").val();
    if (inode_limit != '') {
        volume.inode_limit = inode_limit;
    }

    var container = {
        'fqdn': fqdn,
    }

    if (placement == 'manual') {
        container.dom0 = dom0;
    }
    container.geo = dc;

    var bootstrapCmd = $("#inputBootstrapCmd").val();
    if (bootstrapCmd != undefined && bootstrapCmd != '' ) {
        container.bootstrap_cmd = bootstrapCmd;
    }

    container.volumes = [ volume, ];

    $.each( mapping, function( key, value ) {
        if (key == 'inputClusterName') { return true; }
        var input = $('#' + key + 'Launch').val();
        if (input != '') {
            container[value.name] = input;
        }
    });

    container.extra_properties = {};
    container.project_id = $("#inputProjectIdLaunch").val();
    container.managing_project_id = $("#inputManagingProjectIdLaunch").val();

    container.cluster = $("#inputClusterNameLaunch").val();
    container.project = project;

    container.secrets = {
        '/etc/yandex/mdb-deploy/deploy_version': {
            'mode': '0644',
            'content': '2'
        },
        '/etc/yandex/mdb-deploy/mdb_deploy_api_host': {
            'mode': '0644',
            'content': 'deploy-api.db.yandex-team.ru'
        }
    };

    $('#modalLaunchContainers').modal('hide');
    $('#inputLaunchConfig').val(JSON.stringify(container, null, 4));
    $('#modalLaunchContainersConfig').modal('show');
});


$("#btnLaunchContainers").on('click', function(evt) {
    evt.preventDefault();
    var options = $('#inputLaunchConfig').val();

    $.ajax( {
        url: '/api/v2/containers/' + JSON.parse(options).fqdn,
        method: 'PUT',
        dataType: 'json',
        contentType: "application/json; charset=utf-8",
        data: options,
        error: function(json) {
            var msg_type = 'danger';
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   JSON.stringify(json['responseJSON']) + ".</div>");
            $('#modalLaunchContainersConfig').modal('hide');
        },
        success: function ( json ) {
            var msg = format_deploy(json);
            var msg_type = 'success';
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   msg + ".</div>");
            $('#modalLaunchContainersConfig').modal('hide');
            $('#containersTable').DataTable().ajax.reload();
        }
    } );
});


var mapping = {
    'inputClusterName': {
        'name': 'cluster_name',
    },
    'inputCpuGuarantee': {
        'name': 'cpu_guarantee',
    },
    'inputCpuLimit': {
        'name': 'cpu_limit',
    },
    'inputMemoryGuarantee': {
        'name': 'memory_guarantee',
        'format': true
    },
    'inputMemoryLimit': {
        'name': 'memory_limit',
        'format': true
    },
    'inputHugetlbLimit': {
        'name': 'hugetlb_limit',
        'format': true
    },
    'inputNetGuarantee': {
        'name': 'net_guarantee',
        'format': true
    },
    'inputNetLimit': {
        'name': 'net_limit',
        'format': true
    },
    'inputIOLimit': {
        'name': 'io_limit',
        'format': true
    },
}


function format_volume_table ( data ) {
    res = '<table class="table" cellpadding="5" cellspacing="0" border="0" style="padding-left:50px;">'+
        '<thead>' +
        '<tr>' +
            '<th>Dom0 path</th>' +
            '<th>Path</th>' +
            '<th>Space</th>' +
            '<th>Inode</th>' +
            '<th>Backend</th>' +
            '<th>Writable</th>' +
        '</tr>'+
        '</thead>';
    for (i = 0; i < data.length; i++) {
        if (data[i].dom0_path.startsWith('/data/')) {
            tr_class = 'success';
        } else {
            tr_class = 'warning';
        }
        if (data[i].read_only == true) {
            writable = '<span class="label label-danger">NO</span>';
        } else {
            writable = '<span class="label label-success">YES</span>';
        }
        res += '<tr class="' + tr_class + '">'+
            '<td>' + data[i].dom0_path + '</td>' +
            '<td>' + data[i].path + '</td>' +
            '<td>' + format_limits(data[i].space_guarantee, data[i].space_limit) + '</td>' +
            '<td>' + format_limits(data[i].inode_guarantee, data[i].inode_limit, false) + '</td>' +
            '<td><span class="label label-info">' + data[i].backend + '</span></td>' +
            '<td>' + writable + '</td>'
        '</tr>';
        }
    res += '</table>';
    return res;
}


function format_volume ( rowData ) {
    var div = $('<div/>')
        .addClass( 'loading' )
        .text( 'Loading...' );

    $.ajax( {
        url: '/api/v2/dom0/' + rowData.dom0,
        dataType: 'json',
        success: function ( json ) {
            div.html('<pre>' + JSON.stringify(json, undefined, 2) + '</pre>').removeClass('loading');
        }
    } );

    return div;
}


function showContainersEditModal() {
    var rows = $('#containersTable').DataTable().rows( { selected: true } ).data();
    if (rows.length == undefined || rows.length != 1) {
        $("#alertResult").html("<div class='alert alert-warning'><button " +
                               "type='button' class='close' data-dismiss=" +
                               "'alert'>&times;</button>Select 1 container." +
                               "</div>");
        return;
    }
    $('#modalEditContainersSmall').html('Editing ' + rows[0].fqdn);

    $.each( mapping, function( key, value ) {
        if (mapping[key]['format']) {
            var v = format_bytes(rows[0][value.name]);
        } else {
            var v = rows[0][value.name];
        }
        document.getElementById(key).placeholder = v;
        document.getElementById(key).value = '';
    });
    $('#modalEditContainers').modal('show');
}


function showContainersLaunchModal() {
    $('#modalLaunchContainers').modal('show');
}


function input_fqdn() {
    var fqdn = $('#inputFQDN').val();
    var cluster = fqdn.split('.')[0].slice(0, -1);
    $('#inputClusterNameLaunch').val(cluster);
    $('#inputDom0Path').val('/data/' + fqdn);
    $('#inputBootstrapCmd').val('/usr/local/yandex/porto/mdb_bionic.sh ' + fqdn);
    if (fqdn != '') {
        $('#inputFQDN').parent().parent().removeClass('has-error');
    }
}


function showContainerDeleteModal() {
    var rows = $('#containersTable').DataTable().rows( { selected: true } ).data();
    if (rows.length == undefined || rows.length != 1) {
        $("#alertResult").html("<div class='alert alert-warning'><button " +
                               "type='button' class='close' data-dismiss=" +
                               "'alert'>&times;</button>Select 1 container." +
                               "</div>");
        return;
    }

    var container = rows[0].fqdn

    $('#modalDeleteContainerLabel').html('Deleting container ' + container);
    $('#inputFQDNDestroy').html(container);
    $('#inputSavePaths').html("");

    var paths = [];
    $.ajax( {
        url: '/api/v2/volumes/' + container,
        dataType: 'json',
        async: false,
        success: function ( json ) {
            for (j = 0; j < json.length; j++) {
                if (paths.indexOf(json[j].path) == -1) {
                    paths.push(json[j].path);
                }
            }
        }
    } );
    paths.sort();

    for (i = 0; i < paths.length; i++) {
        $('#inputSavePaths').append(paths[i] + "\n");
    }

    $('#modalDeleteContainer').modal('show');
}


$("#btnDeleteContainer").on('click', function(evt) {
    evt.preventDefault();
    var containers = $('#inputFQDNDestroy').val().split("\n");
    for (i = containers.length - 1; i >= 0; i--) {
        containers[i] = containers[i].trim();
        if (containers[i] == "") {
            containers.splice(i, 1);
        }
    }

    var save_paths = $('#inputSavePaths').val().split("\n");
    for (i = save_paths.length - 1; i >= 0; i--) {
        save_paths[i] = save_paths[i].trim();
        if (save_paths[i] == "") {
            save_paths.splice(i, 1);
        }
    }

    $.ajax( {
        url: '/api/v2/containers/' + containers.toString() + '?save_paths=' + save_paths.toString(),
        method: 'DELETE',
        dataType: 'json',
        error: function(json) {
            var msg_type = 'danger';
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   JSON.stringify(json) + ".</div>");
            $('#modalDeleteContainer').modal('hide');
        },
        success: function ( json ) {
            var msg_type = 'success';
            var msg = format_deploy(json);
            $("#alertResult").html("<div class='alert alert-" + msg_type +
                                   "'><button type='button' class='close' " +
                                   "data-dismiss='alert'>&times;</button>" +
                                   msg + ".</div>");
            $('#modalDeleteContainer').modal('hide');
            $('#containersTable').DataTable().ajax.reload();
        }
    } );
});
