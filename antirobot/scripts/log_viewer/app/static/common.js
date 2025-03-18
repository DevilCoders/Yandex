function SwitchPage(url) {
    function Value(name)
    {
        item = document.getElementById(name);
        if (item)
            return item.value;
        else
            return '';
    }

    url += '?date=' + Value('date') + '&ip=' + Value('ip');

    v = Value('yandexuid');
    if (v != '')
        url += '&yandexuid=' + v;

    v = Value('substring');
    if (v != '')
        url += '&substring=' + v;

    window.location.href = url;
}

function ShowError(msg, full_msg, fn_repeat) {
  $('#errorbox .text').html(msg)
  $('#errorbox .full-error-text').html(full_msg ? full_msg : '')

  let btn = $('#errorbox button.repeat')
  if (fn_repeat) {
    btn.one('click', fn_repeat).show()
  } else {
    btn.hide()
  }
  $('#errorbox').show()
}

function HideError() {
  $('#errorbox').hide()
}

function ShowPendingInfo() {
  $('#pending').show()
  $("#pending .progressbar").progressbar({value: 0});
}

function HidePendingInfo() {
    $('#pending').hide();
    //$("#progressbar").hide();
}

function HandleServerResult(result) {
  if (result.status == 'ready') {
    let tbl = $('#tbl-result')
    AppendRows(tbl, result.rows)
    if (!result.at_end) {
      SetupScroll(result.next_url)
      $(window).scroll()
    } else {
      $('#end-data').show()
    }
  } else if (result.status == 'pending') {
    $( "#progressbar" ).progressbar({
      value: false
    })
    InitPooling(result.next_url)

  } else if (result.status == 'error') {
    if (result.retry_url) {
      ShowError(result.descr, result.full_descr, function() {
        HideError()
        AppendData(result.retry_url)
      })
    } else {
      ShowError(result.descr, result.full_descr)
    }
  }
}

function SetupScroll(url) {
  window.console.log('SetupScroll()')
  $(window).scroll(function () {
    window.console.log('scroll event')

    if ($(window).scrollTop() >= $(document).height() - $(window).height() - 10) {
      $(window).off("scroll")
      AppendData(url)
    }
  })
}

function AppendData(url) {
  let waiting = $('.loading:last')
  HideError()

  waiting.show()
  $.getJSON(url).done(function(result) {
    waiting.hide()
    HandleServerResult(result)
  }).error(function(response, statusTxt, errorTxt) {
    let msg = "The following error has occured while processing the request:<br>" +
          "<span class='system-message'>" +
          statusTxt + ': ' + errorTxt +
          "</span>"
    waiting.hide()
    ShowError(msg)
    HidePendingInfo()
  });
}

function InitPooling(pendingUrl) {
    function CheckUrl() {
        $('.loading-indicator').css("display", "inline-block")
        $.get(pendingUrl)
            .done(function(result) {
                $('.loading-indicator').hide()
                if (result.status == 'ready') {
                    window.location.reload(true);
                } else if (result.status == 'pending') {
                    var timestamp = '<unknown>';
                    if (result.timestamp) {
                        var d = new Date(parseInt(result.timestamp) * 1000);
                        timestamp = d.toLocaleString({ hour12: false });
                    }
                    $("#pending .progress-label").html(result.label);
                    $("#pending .progressbar").progressbar({value: result.progress ? result.progress : 0});
                    $("#pending .timestamp").html(timestamp);
                    setTimeout(CheckUrl, 10000);
                } else if (result.status == 'cancel') {
                    HidePendingInfo();
                    var text = 'The pending opration has been canceled. Try to refresh the page.'
                    if (result.descr) {
                        text += '<br>Reason: ' + result.descr;
                    }
                    ShowError(text);
                } else { // status == error || invalid or somethong unknown
                    HidePendingInfo();
                    ShowError(result.descr ? result.descr : 'Unexpected error on the page. Please, reload the page.', result.full_descr);
                }})

            .fail(function(result) {
                $('.loading-indicator').hide()
                HidePendingInfo();
                ShowError('Could not contact the server');
            });
    }
    ShowPendingInfo()
    CheckUrl();
}

function AppendRows(tbl, rows) {
  /*
    <rows> must look like
  [
    {
      'cols': [
        'col1', 'col2', ... 'colN'
      ],
      'cls': 'tr class value'
    },
    {
      // row 2
    }
    ...
  ]
  */

  let thead = $('thead', tbl)
  let tbody = $('tbody', tbl)

  if (rows.length) {
    thead.show()
  }

  rows.forEach(function(row) {
    let tr = $('<tr>')
    if (row.cls) {
      tr.addClass(row.cls)
    }

    row.cols.forEach(function(col) {
      tr.append($('<td>').append(col)
    )})
    tbody.append(tr)
  })
}
