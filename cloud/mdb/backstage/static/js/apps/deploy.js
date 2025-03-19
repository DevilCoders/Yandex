const state_filters = {
    'all': {},
    'with_changes': {'possible_changes': 'True'},
    'success':  {'result': 'True'},
    'failed': {'result': 'False'},
    'failed_parents': {'result': 'False', 'requisite_failed': 'False'}
}

function filter_state_table(name) {
    let filters = state_filters[name];
    $('#states_table > tbody  > tr').each(function(index, tr) {
        selector = $(tr);
        if (jQuery.isEmptyObject(filters)) {
            selector.show();
        } else {
            let checks = [];
            for (const [key, value] of Object.entries(filters)) {
                let filter_value = selector.data(`filter-${key}`);
                checks.push(filter_value === value);
            }
            if (checks.every(Boolean)) {
                selector.show();
            } else {
                selector.hide();
            }
        }
    });
    $('#state_filters_buttons').children('button').each(function () {
        selector = $(this);
        if (selector.data('states-filter') == name) {
            selector.addClass('btn-primary');
        } else {
            selector.removeClass('btn-primary');
        }
    });
    const url = new URL(window.location.href);
    url.searchParams.delete('states_filter');
    url.searchParams.set('states_filter', name);
    window.history.replaceState(null, null, url);
}
