var lastBtn = null;
var requestInProcess = false;
function getPopup(btn_url) {
    if (requestInProcess) {
        return false;
    }
    if (lastBtn != btn_url) {
        // TODO: need atomic
        requestInProcess = true;
        var prevBtn = lastBtn;
        lastBtn = btn_url;
        $.ajax({url: btn_url,
                cache: false,
                success: function (data) { showForm(data, btn_url); },
                error: function(xhr) {
                    lastBtn = prevBtn;
                    try {
                        alertError(xhr.status);
                    } catch (e) { }
                },
                complete: function(xhr) { requestInProcess = false; }
               })
    } else {
        $("#myModal").dialog("open");
    }
    return false;
}

function showForm(data, btn_url) {
    $("#myModal").remove();
    $("#modal_container").html(data);
    $("#modal_submit_btn").click(function () { return postForm(btn_url); });
    $(".selectpicker").selectpicker();
    $("[data-dismiss=\"modal\"]").click(function () { $("#myModal").dialog("close");} );
    $(".errorlist li").addClass("alert alert-error").css("list-style-type", "none");
    $("#myModal").dialog({
        modal: true,
        open: function () {
            $(".modal-backdrop").remove();
            $(".ui-widget-overlay").css("background-color", "#000000");
            $(".ui-widget-overlay").css("opacity", 0.8);
            $(".ui-widget-overlay").css("position", "fixed");
            $(".ui-widget-overlay").css("left", "0");
            $(".ui-widget-overlay").css("top", "0");
            $(".ui-widget-overlay").css("width", "100%");
            $(".ui-widget-overlay").css("height", "100%");
        },
        create: function () { $(".ui-dialog-titlebar").hide(); },
        closeOnEscape: true
    });
}

function postForm(btn_url) {
    $("#modal_submit_btn").unbind();
    $("#modal_submit_btn").click(function () { return false; });
    var formData = new FormData($("#modal_form")[0]);
    $.ajax({url: btn_url,
            type: "POST",
            success: function(data) {
                location.replace(location.href.replace(location.search, "") + data);
            },
            error: function(xhr) {
                showForm(xhr.responseText, btn_url);
                try {
                    alertError(xhr.status);
                } catch (e) { }
            },
            data: formData,
            cache: false,
            contentType: false,
            processData: false
           });
   return false;
}
