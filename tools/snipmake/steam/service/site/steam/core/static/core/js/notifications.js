$(function () {
    checkNotifications();
});

function checkNotifications() {
    $.ajax({url: "/ajax/notifications/",
            cache: false}).done(function(data) {
                                    $("#notif_container").html(data);
                                    $(".close-notif").click(closeNotification);
                                });
    setTimeout(checkNotifications, 5 * 60 * 1000);
}

function closeNotification() {
    $.ajax({url: "/ajax/close_notification/" + this.id.split("_")[1] + "/",
            cache: false}).done(
                function(data) {
                    $("#close_" + data).parent().remove();
                }
            );
    return false;
}
