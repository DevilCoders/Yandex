function hightlight_menu(name) {
    $('#noodle-menu').find('li').each(function() {
        var menu = $(this).data('noodle-menu');
        if (menu !== name) {
            $(this).removeClass("active");
        } else {
            $(this).addClass("active");
        }
    })
}

/* noodle setup */
var REQUEST_CONTAINER_ID = "#content";
var REQUEST_LABEL_CONTAINER_ID = "#backstage-label";
var REQUEST_ACTIONS_CONTAINER_ID = "#actions";
var GLOBAL_LOADER_CONTAINER_ID = "#loadmodal";
var ERROR_MODAL_CONTAINER_ID = "#error-modal-container";
var ERROR_MODAL_ID = "#error_modal";
var LOAD_CLOCK_CONTAINER_ID = "#loadclock";
var HISTORY_NAME = "MDB Control Plane";
var MODAL_FORM_CONTAINER_ID = "#formmodal";
var DATA_TOGGLE_NAME = "backstage";
var MENU_HL_FUNC = hightlight_menu;
var BASE_HTML_TITLE = 'MDB Control Center'
/* eof noodle setup */

var user_profile_object = {};

bootbox.setDefaults({
    animate: false,
});


function on_success_refresh_container(args) {
    $('#infomodal').modal('hide');
    if (args.response.result) {
        notify('success', args.response.messages, args);
        refresh_container('content')
    } else {
        args.on_error(args);
    }
}


function activate_action_buttons_panel(array) {
    $("#action_buttons_panel_objects_count").text(array.length);

    if (array.length > 0) {
        $("#action_buttons_panel li button").each(function() {
            $(this).prop('disabled', false);
        })
    } else {
        $("#action_buttons_panel li button").each(function() {
            $(this).prop('disabled', true);
        })
    }
}

function save_profile() {
    function on_success(args) {
        if (args.response.result) {
            window.location.href = '/ui/main/user/profile';
        } else {
            args.on_error(args);
        }
    }
    var settings = get_filter_params('user_profile_object');
    settings.timezone = $('#select_timezone').select2('data')[0].id;
    request({
        url: '/ui/main/ajax/user/profile',
        request_type: 'POST',
        params: {
            settings: settings,
        },
        show_global_loader: true,
        on_success: on_success,
    })
}

function show_sql(value) {
    let sql = base64_decode_unicode(value);
    let html = `<div class="modal-dialog modal-lg">
      <div id="infomodal-content" class="modal-content">
        <div class="modal-header">
          <button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>
          <h4><strong>SQL</strong></h4>
        </div>
        <div class="modal-body">
          <div class="row">
             <div id="monaco_sql"></div>
          </div>
        </div>
        <div class="modal-footer">
          <button type="button" class="btn btn-sm btn-default" data-dismiss="modal">Close</button>
        </div>
      </div>
    </div>`
    $('#infomodal').html(html)
    $('#infomodal').modal('show');
    init_monaco({
        container: 'monaco_sql',
        data: sql,
        lang: 'sql',
        autoheight_max_lines: 30,
    })
}

function update_badge_count(key, number) {
    let data = `<span class="badge noodle-tab-badge">${number}</span>`
    $(`#tab_${key}_counter`).html(data);
}

function get_object_action_dialog(app, model, action, array_name) {
    let url = `/ui/${app}/ajax/${model}/dialogs/${action}`
    let array = window[array_name]

    if (array === undefined || array.length == 0) {
        return
    }

    function on_success(args) {
        if (args.response.result) {
            $('#infomodal').html(args.response.html);
            $('#infomodal').modal('show');
            input_focus_init();
        } else {
            args.on_error(args);
        }
        $('*[data-input-type="action"]').keypress(function(e) {
            if (e.which == 13) {
                let btn_id = $(e.target).data('input-btn')
                $(`#${btn_id}`).click()
            }
        });

    }

    request({
        url: url,
        request_type: 'POST',
        params: {
            array_name: array_name,
            app: app,
            model: model,
            ids: array,
        },
        on_success: on_success,
    });
}

function get_single_object_action_dialog(object_pk, app, model, action, array_name) {
    window[array_name] = [object_pk];
    get_object_action_dialog(app, model, action, array_name)
}

function do_object_action(app, model, action, array_name) {
    let url = `/ui/${app}/ajax/${model}/actions/${action}`
    let array = window[array_name]
    let inputs_ok = true

    function on_error(args) {
        default_error_request_handler(args);
        refresh_container('content');
        if (args.response.messages.length > 0) {
            notify('success', args.response.messages, args);
        }
    }

    function on_http_error(args) {
        default_http_error_handler(args);
    }

    inputs = {}
    $("[data-input-type='action']").each(function() {
        let input = $(this)
        let required = input.data('input-required')
        let name = input.data('input-name')
        let value = input.val()

        let label = $(`#label_${name}`)
        let label_error = $(`#label_${name}_error`)

        if (required && !value) {
            inputs_ok = false
            label.addClass('red')
            input.addClass('input-red-border')
            label_error.html('this field is required')
            label_error.show()
        } else {
            inputs[name] = value;
            label.removeClass('red')
            input.removeClass('input-red-border')
            label_error.hide()
        }
    });

    if (inputs_ok) {
        $('#infomodal').modal('hide');

        request({
            url: url,
            request_type: 'POST',
            params: {
                array_name: array_name,
                app: app,
                model: model,
                ids: array,
                inputs: inputs,
            },
            on_success: on_success_refresh_container,
            on_error: on_error,
            on_http_error: on_http_error,
        });
    }
}

function get_health_ua_data(data) {
  if (data) {
    request({
        container: $("#section-content"),
        url: data.params.url,
        href: data.params.href,
        request_type: 'POST',
        params: data.params,
        show_global_loader: false,
        show_container_loader: true,
    })
  }
}

$(function () {
    document.addEventListener('mouseover', function(e) {
      var el = e.target;
      if (!el.matches('[data-staff]')) {
        return;
      }

      var inited = el.dataset.staffInited;
      if (!inited) {
        var login = el.dataset.staff;
        var card = new StaffCard(el, login);
        card.onHover();
        el.dataset.staffInited = true;
      }
    });

});


