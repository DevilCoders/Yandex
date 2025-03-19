var BASE_HTML_TITLE = 'noodle';
var SELECTED_CHECKBOX = '<i class="fas fa-check-square fa-lg noodle-checkbox-selected" aria-hidden="true"></i>';
var UNSELECTED_CHECKBOX = '<i class="fas fa-square fa-lg noodle-checkbox" aria-hidden="true"></i>';
/* stmpwatch */
var timeBegan = null
var timeStopped = null
var stoppedDuration = 0
var started = null;

var pressed_keys = {
    shift: false
};

function start_load_stopwatch() {
    $(LOAD_CLOCK_CONTAINER_ID).html('00.000');
    if (timeBegan === null) {
        timeBegan = new Date();
    }

    if (timeStopped !== null) {
        stoppedDuration += (new Date() - timeStopped);
    }
    started = setInterval(stopwatch_running, 10);
}

function print(value) {
    console.log(value);
}

function stop_load_stopwatch() {
    timeStopped = new Date();
    clearInterval(started);
}

function reset_load_stopwatch() {
    clearInterval(started);
    stoppedDuration = 0;
    timeBegan = null;
    timeStopped = null;
}

function stopwatch_running(){
    var currentTime = new Date()
    var timeElapsed = new Date(currentTime - timeBegan - stoppedDuration)
    var sec = timeElapsed.getUTCSeconds()
    var ms = timeElapsed.getUTCMilliseconds();
    $(LOAD_CLOCK_CONTAINER_ID).html(sec + "s " + (ms > 99 ? ms : ms > 9 ? "0" + ms : "00" + ms) + 'ms');
};
/* eof stopwatch */
function copy_to_clipboard(value) {
    var el = document.createElement('textarea');
    el.value = value;
    el.setAttribute('readonly', '');
    el.style.position = 'absolute';
    el.style.left = '-9999px';
    document.body.appendChild(el);
    el.select();
    document.execCommand('copy');
    document.body.removeChild(el);

}

function lzero(value) {
    return (value < 10 ? '0' : '') + value;
}

function get_str_time() {
    var dt = new Date();
    var time = lzero(dt.getHours()) + ":" + lzero(dt.getMinutes()) + ":" + lzero(dt.getSeconds());
    return time
}

function refresh_container(container_id) {
    container = $('#' + container_id);
    data_array_elem = $('#' + container_id + ' th[data-array_name]')[0];
    if (data_array_elem) {
        window[data_array_elem.getAttribute('data-array_name')] = [];
    }
    get_onload_data(container);
};

function get_csrf_token() {
    name = 'csrftoken';
    var cookie_value = null;
    if (document.cookie && document.cookie != '') {
        var cookies = document.cookie.split(';');
        for (var i = 0; i < cookies.length; i++) {
            var cookie = jQuery.trim(cookies[i]);
            if (cookie.substring(0, name.length + 1) == (name + '=')) {
                cookie_value = decodeURIComponent(cookie.substring(name.length + 1));
                break;
            }
        }
    }
    return cookie_value;
}

function get_onload_data(container) {
    if (container) {
        container.find("[data-onload='True']").each(function() {
            var selector = $(this)
            var url = selector.data('onload-url')
            var params = selector.data('onload-params')
            var small_container_loader = selector.data('small-container-loader');
            get_content_part(selector, url, params, small_container_loader);
        });
    } else {
        $("[data-onload='True']").each(function() {
            var selector = $(this)
            var url = selector.data('onload-url')
            var params = selector.data('onload-params')
            var small_container_loader = selector.data('small-container-loader');
            get_content_part(selector, url, params, small_container_loader);
        });
    }
}

function get_content_part(container, url, params, small_container_loader) {
    function on_success(args) {
        if (args.response.result) {
            args.container.html(args.response.html);
            notify_content_changed(args.container);
            input_focus_init();
            init_tooltip();
            init_sortable_table();
            get_onload_data(container)
        } else {
            args.on_error(args);
        }
    }
    request({
        container: container,
        url: url,
        show_global_loader: false,
        show_container_loader: true,
        small_container_loader: small_container_loader,
        on_success: on_success,
        on_error: inline_error_request_handler,
        on_http_error: inline_http_error_handler,
        params: params
    });

}

function highlight_menu(name) {
    if (name) {
        $('#noodle-menu').find('li').each(function() {
            var menu = $(this).data('noodle-menu');
            if (menu !== name) {
                $(this).removeClass("active");
            } else {
                $(this).addClass("active");
            }
        })
    }
}

function get_simple_form(args) {
    function on_success(args) {
        args.push_history = false
        default_success_request_handler(args);
        if (args.response.result) {
            args.container.modal('show').html(args.response.html)
            if (args.do_not_initialize_select2) {
            } else {
                init_select2();
            };
        };
    };
    request({
        container: $(MODAL_FORM_CONTAINER_ID),
        url: args.form_url,
        params: args.form_params,
        on_success: on_success,
        do_not_initialize_select2: args.do_not_initialize_select2,
    });
}

function default_error_request_handler(args) {
    notify('error', args.response.errors, args);
}

function notify(type, messages, args) {
    /*
        Show notify.
    */
    var args = args || {};
    if (type == 'error') {
        var notify_delay = 0;
    } else {
        var notify_delay = args.notify_delay || 10000;
    }
    var m = '';
    for (i = 0; i < messages.length; i++) {
        msg = messages[i];
        m = m + msg + '<br>';
    }
    if (type == 'success') {
        type = 'success';
        title = '<b>Success</b> <span style="font-size:10px">' + get_str_time() + '</span><br>';
        icon = 'glyphicon glyphicon-ok';
    } else {
        type = 'danger';
        title = '<b>Error</b> <span style="font-size:10px">' + get_str_time() + '</span><br>';
        icon = 'glyphicon glyphicon-exclamation-sign';
    }
    $.notify({
        title: title,
        message: m,
        icon: icon,
    },{
        type: type,
        delay: notify_delay,
        allow_dismiss: true,
        mouse_over: 'pause',
        placement: {
            from: "top",
            align: "right",
        },
        offset: {
            x: 20,
            y: 150,
        },
        animate: {
            enter: 'animated bounceInRight',
            exit: 'animated bounceOut'
        },
    });
    return 0;
}


function submit_simple_form(args) {
    function on_success(args) {
        if (args.response.result) {
            if (args.response.valid) {
                if (args.container_is_modal) {
                    args.container.modal('hide');
                } else {
                    args.container.html(args.response.html);
                }
                if (args.show_notify) {
                    notify('success', args.response.messages, args);
                }
                if (args.callback) {
                    args.callback(args.response);
                }
            } else {
                args.container.html(args.response.html);
            }
        } else {
            args.on_error(args);
        }
        input_focus_init();
    };
    function on_error(args) {
        args.container.modal('hide');
        default_error_request_handler(args);
    }
    function on_http_error(args) {
        args.container.modal('hide');
        default_http_error_handler(args);
    }
    form = $("#" + args.form_name)
    var params = {
        form_data: form.serializeArray(),
    };
    args.selector.button('loading');

    var show_container_loader = false;
    if (!args.container_is_modal) {
        show_container_loader = true;
    }

    request_params = {
        container: args.container,
        url: args.form_url,
        request_type: 'POST',
        params: params,
        on_success: on_success,
        on_error: on_error,
        on_http_error: on_http_error,
        show_global_loader: false,
        show_container_loader: show_container_loader,
        show_notify: args.show_notify,
        container_is_modal: args.container_is_modal,
    }
    var callback_func_name = args.form_name + "_callback";
    if (typeof window[callback_func_name] === "function") {
        request_params['callback'] = window[callback_func_name];
    } else {
        console.log("Warning! Callback function for form " + args.form_name + " was not found");
    }
    request(request_params);
}


function push_history(url) {
    if (window.location.href != url) {
        window.history.pushState('', HISTORY_NAME,  url);
    };
}

function default_success_request_handler(args) {
    if (args.response.result) {
        if (args.container) {
            args.container.html(args.response.html);
        }
        if (args.response.actions) {
            args.actions_container.html(args.response.actions);
        }
        if (args.push_history) {
            push_history(args.href);
        };
        if (args.response.label) {
            args.label_container.html(args.response.label);
        };
        if (args.response.data) {
            highlight_menu(args.response.data.menu);
        }
        if (args.response.html_title) {
            var title = args.response.html_title + ' - ' + BASE_HTML_TITLE;
            document.title = title;
        }
        if (args.container) {
            get_onload_data(args.container)
        }
    } else {
        args.on_error(args);
    }
}

function on_success_modal_refresh_content(args) {
    $('#infomodal').modal('hide');
    if (args.response.result) {
        notify('success', args.response.messages, args);
        refresh_container('content')
    } else {
        args.on_error(args);
    }
}

function reset_selectable() {
    $('*[data-' + DATA_TOGGLE_NAME + '-toggle="checkbox"][data-type="controller"]').each(function() {
        window[$(this).data('array_name')] = [];
    })
}

function get_window_array(array_name) {
    var array = window[array_name];
    if (array == undefined) {
        window[array_name] = [];
        array = window[array_name];
    }
    return array;
}

function select_cell(cell, row, array, key) {
    cell.html(SELECTED_CHECKBOX);
    cell.data('checked', true);
    row.addClass('noodle-row-selected');
    push_to_array(array, key);

}
function unselect_cell(cell, row, array, key) {
    cell.html(UNSELECTED_CHECKBOX);
    cell.data('checked', false);
    row.removeClass('noodle-row-selected');
    remove_from_array(array, key);
}

function process_checkbox_toggle(args) {
    var array = get_window_array(args.array_name);
    var row = $('#row_' + args.key);
    if (args.checked) {
        unselect_cell(args.cell, row, array, args.key);
    } else {
        select_cell(args.cell, row, array, args.key);
    }
    if (args.callback_func) {
        window[args.callback_func](array);
    }

    var controller = $('[data-identifier="' + args.identifier + '"][data-type="controller"]')
    var total = $('[data-identifier="' + args.identifier + '"][data-type="item"]').length
    if (total == array.length) {
        controller.data('checked', true);
        controller.html(SELECTED_CHECKBOX);
    } else {
        controller.data('checked', false);
        controller.html(UNSELECTED_CHECKBOX);
    }
}

function process_controller_checkbox_toggle(args) {
    window[args.array_name] = [];
    var array = get_window_array(args.array_name)
    if (args.checked) {
        args.cell.data('checked', false);
        args.cell.html(UNSELECTED_CHECKBOX);
        $('[data-identifier="' + args.identifier + '"][data-type="item"]').filter(':visible').each(function() {
            var key = $(this).data('key');
            var row = $('#row_' + key);
            unselect_cell($(this), row, array, key);
        });
    } else {
        args.cell.html(SELECTED_CHECKBOX);
        args.cell.data('checked', true);
        $('[data-identifier="' + args.identifier + '"][data-type="item"]').filter(':visible').each(function() {
            var key = $(this).data('key');
            var row = $('#row_' + key);
            select_cell($(this), row, array, key);
        });
    }
    if (args.callback_func) {
        window[args.callback_func](array);
    }
}

function default_http_error_handler(args) {
    $(ERROR_MODAL_CONTAINER_ID).html('<h4>' + args.XMLHttpRequest.status + ': ' + args.errorThrown + '</h4>requested url: ' + args.XMLHttpRequest.url);
    $(ERROR_MODAL_ID).modal('show');
}

function inline_http_error_handler(args) {
    let html = `
    <div class="row">
      <div class="col-lg-24">
        <div class="noodle-http-error">
          <span class="noodle-http-error-header">${args.XMLHttpRequest.status}: ${args.errorThrown}</span><br>
          ${args.XMLHttpRequest.url}<br>
        </div>
      </div>
    </div>`
    args.container.html(html)
}

function inline_error_request_handler(args) {
    if (args.response.html) {
        var html = args.response.html;
    } else {
        var html = "<span><i class='fa fa-exclamation-triangle fa-lg fa-red'></i> Error: " + args.response.errors.join(', ')
    }
    args.container.html(html)
}

function get_default_if_undefined(value, default_value) {
    if (value == undefined) {
        return default_value;
    } else {
        return value
    }
}

function request(args) {
    /* Function to make http requests to rackroll */

    /*
    args.url:
      url for http request
    args.label:
      current page label
    */

    /* container to work with (jquery selector) */
    args.container = args.container || $(REQUEST_CONTAINER_ID)

    /* function to call if response['result'] is true */
    args.on_success = args.on_success || default_success_request_handler;

    /* function to call if response['result'] is false */
    args.on_error = args.on_error || default_error_request_handler;

    /* function to call on http error */
    args.on_http_error = args.on_http_error || default_http_error_handler;

    /* url for browser bar and history */
    args.href = args.href || window.location.href;

    /* params to pass with request */
    args.params = args.params || {};

    /* container to store label to */
    args.label_container = args.label_container || '';

    /* container to store actions to */
    args.actions_container = args.actions_container || '';

    /* http request type */
    args.request_type = args.request_type || 'GET';

    if (args.push_history == undefined ) {
        args.push_history = true;
    }

    /* show global loader or not */
    if (args.show_global_loader == undefined ) {
        args.show_global_loader = true;
    }

    /* show container loader or not */
    if (args.show_container_loader == undefined ) {
        args.show_container_loader = false;
    }

    /* show project loader or not */
    if (args.show_project_loader == undefined ) {
        args.show_project_loader = false;
    }

    /* container loader size */
    if (args.small_container_loader == undefined ) {
        args.small_container_loader = false;
    }

    /* show notify or not */
    if (args.show_notify == undefined ) {
        args.show_notify = true;
    }

    /* show notify or not */
    if (args.container_is_modal == undefined ) {
        args.container_is_modal = false;
    }

    var request_data;
    if (jQuery.isEmptyObject(args.params)) {
        if (args.request_type === "POST" || args.request_type === "DELETE" || args.request_type === "PUT") {
            request_data = {
                csrfmiddlewaretoken: get_csrf_token(),
            }
        } else {
            request_data = '';
        };
    } else {
        if (args.request_type === "POST" || args.request_type === "DELETE" || args.request_type === "PUT") {
            request_data = {
                csrfmiddlewaretoken: get_csrf_token(),
                data: JSON.stringify({
                   params: args.params,
                }),
            };
        } else {
            request_data = args.params
        };
    };

    show_global_loader(args);
    show_container_loader(args);
    show_project_loader(args);
    $.ajax({
        args: args,
        url: args.url,
        type: args.request_type,
        dataType: 'json',
        data: request_data,
        beforeSend: function(XMLHttpRequest, settings) {
            XMLHttpRequest.args = args;
            XMLHttpRequest.url = settings.url;
            XMLHttpRequest.setRequestHeader("X-CSRFToken", get_csrf_token());
        },
        success: function(data) {
            hide_global_loader(args);
            hide_project_loader(args);
            args.response = data
            args.on_success(args);
        },
        error: function(XMLHttpRequest, textStatus, errorThrown) {
            var args = XMLHttpRequest.args;
            hide_global_loader(args);
            hide_project_loader(args);
            args.XMLHttpRequest = XMLHttpRequest;
            args.textStastus = textStatus;
            args.errorThrown = errorThrown;
            args.on_http_error(args);
        },
    });
}

function simple_request(args) {
    function on_success(args) {
        if (args.response.result) {
            notify('success', args.response.messages, args);
        } else {
            args.on_error(args);
        }
    };

    if (args.request_type == undefined ){
        args.request_type = 'GET'
    }
    request({
        url: args.url,
        request_type: args.request_type,
        on_success: on_success,
        show_global_loader: true,
    })
}

function simple_post_request(url) {
    return simple_request({'url': url, 'request_type': 'POST'})
}

function show_global_loader(args) {
    /* show global loader with spinner and timer */
    if (!args.show_global_loader) {
        return
    }
    $(GLOBAL_LOADER_CONTAINER_ID).modal('show');
    start_load_stopwatch();
}

function show_container_loader(args) {
    if (!args.show_container_loader) {
        return
    }
    if (args.small_container_loader) {
        loader = '<i class="fas fa-circle-notch fa-spin" style="color: #027bf3"></i>'
    } else {
        loader = '<div class="loader-box"><div class="loader"></div></div>';
    }
    args.container.html(loader);
}

function show_project_loader(args) {
    if (!args.show_project_loader) {
        return
    }
    var container = $("#loader-box-project");
    container.show();
}

function hide_project_loader(args) {
    if (!args.show_project_loader) {
        return
    }
    var container = $("#loader-box-project");
    container.hide();
}

function hide_global_loader(args) {
    /* hide global loader and stop timerr */
    if (!args.show_global_loader) {
        return
    }
    $(GLOBAL_LOADER_CONTAINER_ID).modal('hide');
    stop_load_stopwatch();
    reset_load_stopwatch();
}


function push_to_array(array, value) {
    var index = array.indexOf(value);
    if (index < 0) {
        array.push(value);
    }
}

function remove_from_array(array, value) {
    var index = array.indexOf(value);
    if (index > -1) {
        array.splice(index, 1);
    }
}

function notify_content_changed(container) {
    if ($(container).data('onload-notify')) {
        $(container).trigger("data-loaded")
    }
}

function input_focus_init() {
    var inputs = {};
    var priorities = [];
    $('[data-input-focus="true"]').each(function(index, input) {
        var priority = $(input).data('input-focus-priority') || 0;
        inputs[priority] = input
        priorities.push(priority);
    })
    max = Math.max(...priorities);
    if (!jQuery.isEmptyObject(inputs)) {
        if (max) {
            inputs[max].focus();
        }
    };
};

function init_select2() {
    $('select').select2({
        theme: 'bootstrap',
        placeholder: "Select an option",
        allowClear: true,
        dropdownParent: $("#formmodal"),
    });
};

function init_tooltip() {
    $('[data-toggle="tooltip"]').tooltip()
}

function init_sortable_table() {
    let data_table_args = {
        'paging': false,
        'searching': false,
        'info': false,
        'order': [],
        'columnDefs': []
    }
    $('.noodle-table-sortable').each(function() {
        if (!$(this).hasClass('dataTable')) {
            if ($(this).hasClass('noodle-table-selectable')) {
                data_table_args['columnDefs'].push({orderable: false, targets: 0})
            }
            $(this).DataTable(data_table_args);
        }
    })
}

function get_filter_params(array_name) {
    var params = {};

    for (var key in window[array_name]) {
        let value = window[array_name][key];
        if (value) {
            if (value.length > 0) {
                if (Array.isArray(value)) {
                    value = value.join(',');
                }
                params[key] = value;
            }
        }
    };
    return params
}

function get_filter_data(filter_id) {
    var filter = $('#' + filter_id);
    var container = $("#" + filter.data('noodle-filter-container'))
    var url = filter.data('noodle-filter-url');
    var href = filter.data('noodle-filter-href');
    var filter_object_name = filter.data('noodle-filter-object-name');
    var params = get_filter_params(filter_object_name);

    return {
        'container': container,
        'url': url,
        'href': href,
        'show_global_loader': false,
        'show_container_loader': true,
        'params': params,
        'on_success': filter_on_success,
        'on_error': inline_error_request_handler,
        'on_http_error': inline_http_error_handler
    }

}
function filter_on_success(args) {
    if (args.response.result) {
        args.container.html(args.response.html);
        notify_content_changed(args.container)
        init_tooltip();
        init_sortable_table();
        get_onload_data(args.container)
        var href = args.href + '?' + $.param(args.params)
        push_history(href);
    } else {
        args.on_error(args);
    }
}

function noodle_apply_filter() {
    var data = get_filter_data('noodle-filter');
    request({
        container: data.container,
        url: data.url,
        href: data.href,
        show_global_loader: data.show_global_loader,
        show_container_loader: data.show_container_loader,
        on_success: data.on_success,
        on_error: data.on_error,
        on_http_error: data.on_http_error,
        params: data.params
    });
};

function display_messages() {
    $('.noodle-message').each(function() {
        var selector = $(this)
        var message = selector.text();
        var severity = selector.data('severity');
        notify(severity, [message]);
    })
}


function init_window_object(obj_name) {
    let obj = window[obj_name];
    if (obj == undefined) {
        window[obj_name] = {};
    }
}


function init_filter() {
    $(document).on('click', '.noodle-filter-name', function(a) {
        a.preventDefault();
        var selector = $(this);
        selector.children().first().toggleClass('noodle-filter-chevron-right noodle-filter-chevron-down');
        selector.parent().find('.noodle-filter-content').toggle(100);
        selector.parent().find('input').focus()
    });

    $('*[data-noodle-filter-type="input-single"]').each(function() {
        var selector = $(this);
        var filter_object_name = selector.data('noodle-filter-object-name');
        var key = selector.data('noodle-filter-key');
        var value = selector.val();

        init_window_object(filter_object_name);
        window[filter_object_name][key] = value;
    });

    $('*[data-noodle-filter-type="input-single"]').bind('input', function() {
        var selector = $(this);
        var filter_object_name = selector.data('noodle-filter-object-name');
        var key = selector.data('noodle-filter-key');
        var value = selector.val();

        init_window_object(filter_object_name);
        window[filter_object_name][key] = value;
        if (value) {
            selector.closest('li').addClass('noodle-filter-item-changed');
        } else {
            selector.closest('li').removeClass('noodle-filter-item-changed');
        }
    });

    $('*[data-noodle-filter-type="input-single"]').keypress(function(e) {
        if (e.which == 13) {
            noodle_apply_filter();
        }
    });

    $('*[data-noodle-filter-type="checkbox-group"]').each(function() {
        var selector = $(this);
        var filter_object_name = selector.data('noodle-filter-object-name');
        var key = selector.data('noodle-filter-key');
        var value = selector.val();

        init_window_object(filter_object_name);
        if (!(key in window[filter_object_name])) {
            window[filter_object_name][key] = [];
        }
        if (selector.is(':checked')) {
            push_to_array(window[filter_object_name][key], value);
        };
    });

    $('*[data-noodle-filter-type="checkbox-group"]').change(function() {
        var selector = $(this);
        var filter_object_name = selector.data('noodle-filter-object-name');
        var key = selector.data('noodle-filter-key');
        var value = selector.val();

        init_window_object(filter_object_name);
        if (selector.is(':checked')) {
            push_to_array(window[filter_object_name][key], value);
        } else {
            remove_from_array(window[filter_object_name][key], value);
        }
        if (window[filter_object_name][key].length == 0) {
            selector.closest('li').removeClass('noodle-filter-item-changed');
        } else {
            selector.closest('li').addClass('noodle-filter-item-changed');
        }
    });

    $('*[data-noodle-dependency]').change(function () {
        var selector = $(this);

        /* disable/enable dependent controls */
        $('*[data-noodle-depends-on~="' + selector.attr('id') +'"]').each(function() {
            /* disable dependend elements if dependency is not checked or disabled */
            $(this).attr('disabled', !selector.is(':checked') || !selector.is(':enabled'))
            /* trigger change event for lower dependants to handle changes in state */
            $(this).trigger('change')
        })
    })

    $('*[data-noodle-dependency]').each(function () {
        /* handle current states */
        $(this).trigger('change')
    });

    $('*[data-noodle-filter-type="radio-group"]').each(function() {
        var selector = $(this);
        var filter_object_name = selector.data('noodle-filter-object-name');
        var key = selector.data('noodle-filter-key');
        var value = selector.val();
        if (selector.is(':checked')) {
            /* array should be defined in app like filter_object_name = []; */
            window[filter_object_name][key] = value;
        }
    });
    $('*[data-noodle-filter-type="radio-group"]').change(function() {
        var selector = $(this);
        var filter_object_name = selector.data('noodle-filter-object-name');
        var key = selector.data('noodle-filter-key');
        var value = selector.val();
        var callback_func = selector.data('callback_func');

        init_window_object(filter_object_name);
        if (selector.is(':checked')) {
            window[filter_object_name][key] = value;
            if (callback_func) {
                window[callback_func]();
            }
        }
        selector.closest('li').addClass('noodle-filter-item-changed');
    })
}

/* codemirror line selection */

function get_line_marker(line_num) {
  var marker = document.createElement("div");
  marker.className = "CodeMirror-linkedlinenumber";
  marker.innerHTML = line_num + 1;
  return marker;
}

function add_line(arr, line_number) {
    var exist = false;
    var index = arr.indexOf(line_number);
    if (index > -1) {
        arr.splice(index, 1);
    }
    arr.push(line_number)
}


function remove_line(arr, line_number) {
    var index = arr.indexOf(line_number);
    if (index > -1) {
        arr.splice(index, 1);
    }
}


function select_line(cm, line_num, arr) {
    cm.addLineClass(line_num, 'wrap', 'CodeMirror-linkedLine')
    cm.setGutterMarker(line_num, "CodeMirror-linenumbers", get_line_marker(line_num));
    add_line(arr, line_num);
}


function unselect_line(cm, line_num, arr) {
    cm.removeLineClass(line_num, 'wrap', 'CodeMirror-linkedLine')
    cm.setGutterMarker(line_num, "CodeMirror-linenumbers", null);
    remove_line(arr, line_num)
}


function process_line_selection(cm, line_num, arr) {
    if (arr.includes(line_num)) {
        unselect_line(cm, line_num, arr);
    } else {
        select_line(cm, line_num, arr)
    }
}


function unselect_all(cm, arr) {
    var selected_lines_copy = [...arr];
    for (var i = 0; i < selected_lines_copy.length; i++) {
        unselect_line(cm, selected_lines_copy[i], arr);
    }
}


function select_range(cm, start, end, arr) {
    for (var i = start; i < end; i++) {
        select_line(cm, i, arr)
    }
}


function update_url(arr, parameter_name) {
    var url = new URL(window.location.href);
    var params = url.searchParams;
    if (arr.length === 0) {
        params.delete(parameter_name);
    } else if (arr.length === 1) {
        params.set(parameter_name, arr[0] + 1)
    } else {
        var first_num = arr[0] + 1
        var last_num = arr[arr.length - 1] + 1
        params.set(parameter_name, first_num + '-' + last_num)
    }
    url.search = params.toString();
    push_history(url.toString());
}


function jump_to_line(cm, line_number) {
    cm.focus();
    cm.setCursor({line: line_number + 25, ch: 0});
    cm.setCursor({line: line_number, ch: 0});
}

function select_lines_from_url(cm, arr, parameter_name) {
    var url = new URL(window.location.href);
    var params = url.searchParams;
    var lines = params.get(parameter_name);
    var focus_line = null;
    if (lines === null) {
        return
    }
    if (lines.includes("-")) {
        var spl = lines.split("-")
        start = Number(spl[0]) -1
        end = Number(spl[1])
        select_range(cm, start, end, arr)
        focus_line = start;
    } else {
        focus_line = Number(lines) -1;
        select_line(cm, focus_line, arr)
    }
    if (focus_line != null) {
        jump_to_line(cm, focus_line, arr)
    }
}

function init_monaco(args) {
    args.lang = get_default_if_undefined(args.lang,'javascript')
    args.readOnly = get_default_if_undefined(args.readOnly, true)
    args.fontSize = get_default_if_undefined(args.fontSize, 13)
    args.wordWrap = get_default_if_undefined(args.wordWrap, true)
    args.contextmenu = get_default_if_undefined(args.contextmenu, true)
    args.scrollBeyondLastLine = get_default_if_undefined(args.scrollBeyondLastLine, false)
    args.automaticLayout = get_default_if_undefined(args.automaticLayout, true)
    args.minimap = get_default_if_undefined(args.minimap, {enabled: true})
    args.folding = get_default_if_undefined(args.folding, true)
    args.lineNumbers = get_default_if_undefined(args.lineNumbers, true)
    args.renderLineHighlight = get_default_if_undefined(args.renderLineHighlight, "all")
    args.matchBrackets = get_default_if_undefined(args.matchBrackets, "always")
    args.autoheight = get_default_if_undefined(args.autoheight, true)
    args.autoheight_max_lines = get_default_if_undefined(args.autoheight_max_lines, 40)

    let model = monaco.editor.createModel(args.data, args.lang);
    let container = document.getElementById(args.container)
    let editor = monaco.editor.create(container, {
        model: model,
        readOnly: args.readOnly,
        fontSize: args.fontSize,
        wordWrap: args.wordWrap,
        contextmenu: args.contextmenu,
        scrollBeyondLastLine: args.scrollBeyondLastLine,
        automaticLayout: args.automaticLayout,
        minimap: args.minimap,
        folding: args.folding,
        lineNumbers: args.lineNumbers,
        renderLineHighlight: args.renderLineHighlight,
        matchBrackets: args.matchBrackets,
    });
    if (args.autoheight) {
        let linesCount = editor.getModel().getLineCount()
        if (args.autoheight_max_lines !== null) {
            if (linesCount > args.autoheight_max_lines) {
                linesCount = args.autoheight_max_lines
            }
        }
        const contentHeight = linesCount * 19
        $("#" + args.container).height(contentHeight);
        editor.layout()
    }
    return editor
}

function init_minimonaco(data, lang, container) {
    return init_monaco({
        container: container,
        lang: lang,
        data: data,
        renderLineHighlight: "none",
        matchBrackets: "never",
        lineNumbers: false,
        folding: false,
        minimap: {enabled: false},
        contextmenu: false,
        wordWrap: false,
    })
}

function init_codemirror(args) {
    args.show_trailing_space = get_default_if_undefined(args.show_trailing_space, false)
    args.line_numbers = get_default_if_undefined(args.line_numbers, true)
    args.read_only = get_default_if_undefined(args.read_only, true)
    args.gutters = get_default_if_undefined(args.gutters, ["CodeMirror-linenumbers"])
    args.theme = get_default_if_undefined(args.theme, "default")
    args.selected_list_name = get_default_if_undefined(args.selected_list_name, "selected_lines")
    args.allow_line_selection = get_default_if_undefined(args.allow_line_selection, false)
    args.lines_url_parameter_name = get_default_if_undefined(args.lines_url_parameter_name, 'L')
    args.json = get_default_if_undefined(args.json, false)
    args.expandtab = get_default_if_undefined(args.expandtab, false)
    args.viewport_margin = get_default_if_undefined(args.viewport_margin, 10)
    args.fromtextarea = get_default_if_undefined(args.fromtextarea, false)

    var container = document.getElementById(args.container)

    if (args.fromtextarea) {
        var editor = CodeMirror.fromTextArea(container, {
            lineNumbers: args.line_numbers,
            showTrailingSpace: args.show_trailing_space,
            readOnly: args.read_only,
            gutters: args.gutters,
            mode: args.mode,
            theme: args.theme,
            json: args.json,
            viewportMargin: args.viewport_margin,
            indentUnit: 4,
        });
        editor.on("change", function(cm, change) {
            container.value = editor.getValue("\n");
        });

    } else {
        var text = container.textContent || container.innerText;
        var editor = CodeMirror(function(node){container.parentNode.replaceChild(node, container)}, {
            value: text,
            lineNumbers: args.line_numbers,
            showTrailingSpace: args.show_trailing_space,
            readOnly: args.read_only,
            gutters: args.gutters,
            mode: args.mode,
            theme: args.theme,
            json: args.json,
            viewportMargin: args.viewport_margin,
            indentUnit: 4,
        });
    }

    if (args.expandtab) {
        editor.setOption("extraKeys", {
            Tab: function(cm) {
                var spaces = Array(cm.getOption("indentUnit") + 1).join(" ");
                cm.replaceSelection(spaces);
            }
        });
    }

    if (args.allow_line_selection) {
        var selected_lines = get_window_array(args.selected_list_name)
        editor.on("gutterClick", function(cm, line_num) {
            if (pressed_keys['shift']) {
                if (selected_lines.length == 0) {
                    process_line_selection(cm, line_num, selected_lines)
                } else {
                    var prev = selected_lines[selected_lines.length - 1]
                    if (prev === line_num) {
                        process_line_selection(cm, line_num, selected_lines)
                        return
                    }
                    if (prev > line_num) {
                        var start = line_num;
                        var end = prev + 1;
                    } else {
                        var start = prev
                        var end = line_num + 1
                    }
                    unselect_all(cm, selected_lines);
                    select_range(cm, start, end, selected_lines);
                }
            } else {
                if (selected_lines.length === 1) {
                    if (selected_lines[0] === line_num) {
                        unselect_line(cm, line_num, selected_lines)
                    } else {
                        unselect_all(cm, selected_lines);
                        select_line(cm, line_num, selected_lines)
                    }
                } else {
                    if (selected_lines.length > 0) {
                        unselect_all(cm, selected_lines);
                    }
                    process_line_selection(cm, line_num, selected_lines)
                }
            }
            update_url(selected_lines, args.lines_url_parameter_name)
        });
        select_lines_from_url(editor, selected_lines, args.lines_url_parameter_name)
    }
    return editor
}
/* eof codemirror line selection */

function collapse_sidebar(selector) {
    var width = '250px';
    var main_padding = '260px';
    var footer_padding = '230px';
    var sidebar = $("#noodle-sidebar");

    if (sidebar.hasClass('noodle-sidebar-lg')) {
        width = '300px';
        main_padding = '310px';
        footer_padding = '280px';
    } else if (sidebar.hasClass('noodle-sidebar-xxl')) {
        width = '450px';
        main_padding = '460px';
        footer_padding = '430px';
    }
    var state = selector.data('state');
    if (state == "opened") {
        $('#noodle-sidebar-head').hide()
        $('#noodle-sidebar-body').hide()
        $('#noodle-sidebar-footer').css("left", "40px");
        $('#noodle-sidebar-collapse').html('<button class="btn btn-xs btn-default noodle-sidebar-collapse-btn"><i class="fa fa-fw fa-chevron-right"></i></button>')
        $("#noodle-sidebar").css('width', '60px');
        $("#noodle-sidebar").css('min-width', '60px');
        $("#noodle-sidebar-main").css('padding-left', '70px');
        selector.data('state', 'closed');
    } else {
        $('#noodle-sidebar-head').show()
        $('#noodle-sidebar-body').show()
        $('#noodle-sidebar-footer').css("left", footer_padding);
        $('#noodle-sidebar-collapse').html('<button class="btn btn-xs btn-default noodle-sidebar-collapse-btn"><i class="fa fa-fw fa-chevron-left"></i></button>')
        $("#noodle-sidebar").css('width', width);
        $("#noodle-sidebar").css('min-width', width);
        $("#noodle-sidebar-main").css('padding-left', main_padding);
        selector.data('state', 'opened');
    }
}

function base64_decode_unicode(value) {
    return decodeURIComponent(atob(value).split('').map(function(c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
    }).join(''));
}

function update_url_for_tab(tab) {
    let href = $(tab).attr('href');
    let tab_id = href.substring(5);
    const url = new URL(window.location.href);
    url.searchParams.delete('tab');
    url.searchParams.set('tab', tab_id);
    window.history.replaceState(null, null, url);
}

$(function () {
    window.onpopstate = function(event) {
        window.location.href = document.location;
    };
    get_onload_data();
    input_focus_init();
    init_filter();
    init_tooltip();
    display_messages();
    $(document).on("click", '#noodle-sidebar-collapse', function(a) {
        var selector = $(this);
        collapse_sidebar(selector);
    });
    $(document).on("click", '*[data-' + DATA_TOGGLE_NAME + '-toggle="checkbox"]', function(a) {
        a.preventDefault();
        var selector = $(this);
        var array_name = selector.data('array_name');
        var callback_func = selector.data('callback_func');
        var key = selector.data('key');
        var type = selector.data('type');
        var checked = selector.data('checked');
        var identifier = selector.data('identifier');
        if (type == 'item') {
            process_checkbox_toggle({
                'cell': selector,
                'array_name': array_name,
                'key': key,
                'checked': checked,
                'identifier': identifier,
                'callback_func': callback_func,
            })
        } else {
            process_controller_checkbox_toggle({
                'cell': selector,
                'array_name': array_name,
                'checked': checked,
                'callback_func': callback_func,
                'identifier': identifier,
            })
        }
    });
    $(document).on("click", '*[data-' + DATA_TOGGLE_NAME + '-toggle="simple-form"]', function (a) {
        a.preventDefault();
        var selector = $(this)
        var form_url = selector.data('form_url');
        var form_params = selector.data('form_params');
        get_simple_form({'form_url': form_url, 'form_params': form_params});
    });
    $(document).on("click", '*[data-noodle-toggle="copy"]', function (a) {
        a.preventDefault();
        var selector = $(this)
        var value = base64_decode_unicode(selector.data('value'));

        copy_to_clipboard(value);

        selector.fadeOut(100, function() {});
        selector.fadeIn(100, function() {});
    });
    $(document).on("click", '*[data-' + DATA_TOGGLE_NAME + '-toggle="simple-form-submit"]', function (a) {
        a.preventDefault();
        var selector = $(this)
        var form_url = selector.data('form_url');
        var form_name = selector.data('form_name');
        var callback_params = selector.data('callback_params');
        var show_notify = selector.data('show_notify');
        if (show_notify == undefined) {
            show_notify = true;
        }
        var container = selector.data('container');
        if (container == undefined) {
            container = $(MODAL_FORM_CONTAINER_ID);
        } else {
            container = $('#' + container);
        }
        var container_is_modal = selector.data('container_is_modal');
        if (container_is_modal == undefined ) {
            container_is_modal = true;
        }
        submit_simple_form({'selector': selector,
                            'form_url': form_url,
                            'form_name': form_name,
                            'callback_params': callback_params,
                            'container': container,
                            'container_is_modal': container_is_modal,
                            'show_notify': show_notify});
    });
    $(document).on('shown.bs.tab', 'a[data-toggle="tab"]', function (e) {
        update_url_for_tab(e.target);
        $.fn.dataTable.tables({ visible: true, api: true }).columns.adjust();
    });
    $(document).on("keypress", '*[data-' + DATA_TOGGLE_NAME + '-type="ajax-form"]', function (e) {
        if ((e.keyCode == 13) && (e.target.type != "textarea")) {
            e.preventDefault();
            form_id = $(this).attr('id')
            submit_button = $("#submit_" + form_id);
            submit_button.trigger('click');
        };
    });
    window.onkeyup = function(e) {
        if (e.shiftKey) {
            pressed_keys['shift'] = false
        }
    }
    window.onkeydown = function(e) {
        if (e.shiftKey) {
            pressed_keys['shift'] = true
        }
    }
});
