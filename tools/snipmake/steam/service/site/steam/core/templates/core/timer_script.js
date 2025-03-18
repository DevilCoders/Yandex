seconds = {{ est.get_elapsed_time }};

function tick() {
    tick_date = new Date(seconds * 1000);
    tick_hours = tick_date.getUTCHours();
    tick_mins = tick_date.getUTCMinutes();
    tick_secs = tick_date.getUTCSeconds();
    ++seconds;
{% if est.status == est.Status.ASSIGNED or est.status == est.Status.SKIPPED %}
    $("#id_time_elapsed").val(seconds);
{% endif %}
    $("#timer").html((tick_hours < 10? "0": "") + tick_hours + ":" +
                    (tick_mins < 10? "0": "") + tick_mins + ":" +
                    (tick_secs < 10? "0": "") + tick_secs);
{% if est.status == est.Status.ASSIGNED or est.status == est.Status.SKIPPED %}
    setTimeout(tick, 1000);
{% endif %}
}
