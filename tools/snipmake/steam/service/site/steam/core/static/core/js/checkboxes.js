function setupControlledCheckboxes(group) {
    $("#" + group + "_controller").change(controlCheckboxes);
    $("[data-group=\"" + group + "\"]:visible").change(restoreController);
}

function controlCheckboxes() {
    group = this.id.split("_")[0];
    controller = $("#" + this.id);
    $("[data-group=\"" + group + "\"]:visible").prop("checked", controller.prop("checked"));
    $("[data-group=\"" + group + "\"]:visible").trigger("change");
}

function restoreController() {
    group = this.getAttribute("data-group");
    controller = $("#" + group + "_controller");
    checkboxes = $("[data-group=\"" + group + "\"]:visible");
    if ($(":checked[data-group=\"" + group + "\"]:visible").length < checkboxes.length) {
        controller.prop("checked", false);
    } else {
        controller.prop("checked", true);
    }
}
