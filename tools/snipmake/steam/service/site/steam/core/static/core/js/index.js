$(function() {
    $(".selectpicker").selectpicker();
    $(".switcher").change(function () {
        $(this).closest("form").submit();
    });
    $("#main_tabs").tabs();
    $(".errorlist").css("margin", "0px");
    $(".errorlist li")
        .addClass("alert alert-error")
        .css("list-style-type", "none");
    $(".uploader").uploader();

    $('[data-toggle="confirmation"]').confirmation();
});
