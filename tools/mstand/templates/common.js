var addVisibilityListener = function (checkboxId, selector) {
    var checkbox = document.getElementById(checkboxId);
    checkbox.addEventListener("change", function() {
        Array.prototype.forEach.call(document.querySelectorAll(selector), function(el) {
            if (checkbox.checked) {
                el.classList.remove("hidden");
            } else {
                el.classList.add("hidden");
            }
        });
    })
};
