{% load i18n %}

stage = "{{ stage }}";
{% if stage != 'diff' %}
    choices = [
        {% for guid_and_color in est.json_value.choice %}
            "{{ guid_and_color }}",
        {% endfor %}
    ];
    merged = [
        {% for seg in est.json_value.merge %}
            "{{ seg }}",
        {% endfor %}
    ];
{% else %}
    choices1 = [
        {% for guid_and_color in est1.json_value.choice %}
            "{{ guid_and_color }}",
        {% endfor %}
    ];
    merged1 = [
        {% for seg in est1.json_value.merge %}
            "{{ seg }}",
        {% endfor %}
    ];
    choices2 = [
        {% for guid_and_color in est2.json_value.choice %}
            "{{ guid_and_color }}",
        {% endfor %}
    ];
    merged2 = [
        {% for seg in est2.json_value.merge %}
            "{{ seg }}",
        {% endfor %}
    ];
{% endif %}

ANNOTATION_ERROR = "{% trans 'Please fill all selectors' %}!";
RESOLVING_ERROR = "{% trans 'Please resolve all conflicts' %}!";
VIEW_CONTENT_TEXT = "{% trans 'View content' %}";
SPLIT_TEXT = "{% trans 'split'|capfirst %}";
JOIN_TEXT = "{% trans 'join'|capfirst %}";
MERGE_TEXT = "{% trans 'merge'|capfirst %}";
UNMERGE_TEXT = "{% trans 'unmerge'|capfirst %}";
UNDO_LAST_SPLIT = "{% trans 'undo last split'|capfirst %}";

HOTKEYS = {
{% for seg_name, hotkey in est.SegmentName.HOTKEYS %}
    {{ hotkey }}: "{{ seg_name }}",
{% endfor %}
};

HtmlBlocks = {
    viewContent: function (guid) {
        return "<input type=\"button\" value=\"{% trans 'View content' %}\" class=\"btn view_button\" id=\"viewButton_" + guid + "\" />";
    },
    currentDescription: function (elem) {
        return "<span class=\"number\">" + (1 + Globals.chosenCount) + "</span>" +
               ". " +  TreeProcessing.description(elem, true);
    },
    colorDiv: function (color) {
            return "<div style=\"display: inline-block; width: 25px; position: relative; top: 12px; height: 25px; border: 1px solid #000000; margin: 5px; background-color: " +
                   color + "; cursor: pointer;\"></div>";
    },
    markHtml: function (guid, color) {
        return "<div id=\"markdiv_" + guid + "\" style=\"pointer-events: none; position: absolute; width: 100%; height: 100%; outline: solid 3px " +
               color + "; z-index: " + (SiteForSegmentation.maxZIndex + 1) +
               "; text-align: left; color: #ffffff; font-family: 'Helvetica Neue',Helvetica,Arial,sans-serif; font-size: 10px; line-height; 12px;\">" +
               "<div style=\"pointer-events: auto; background-color: rgba(0, 0, 0, 0.5); cursor: pointer; float: left;\">" + (Globals.chosenCount + 1) +
               "</div></div>";
    },
    annotateMark: function (guid) {
       return "<div id=\"annotate_mark_" + guid + "\" style= \" width: 40px\"></div>";
    },
    splitButton: function () {
        return "<input type=\"button\" class=\"btn split_btn\" value=\"" + SPLIT_TEXT + "\" />";
    },
    joinButton: function () {
        return "<input type=\"button\" class=\"btn join_btn\" value=\"" + JOIN_TEXT + "\" />";
    },
    mergeButton: function () {
        return "<p style=\"text-align: center; margin-top: -21px; margin-bottom: 0px;\">" +
               "<input type=\"button\" class=\"btn merge_btn\" value=\"" + MERGE_TEXT +
               "\" style=\"padding-top: 0px; padding-bottom: 0px;\" />" + "</p>";
    },
    divInline: function () {
        return "<div style=\"margin: 0px; padding: 0px; display: inline;\"></div>";
    },
    submitInput: function (guid) {
        return "<input type=\"hidden\" id=\"id_choice_" + guid +
               "\" name=\"choice\" value=\"" + guid + "\" />";
    },
    segTypeInput: function (beginning, end, segType) {
        return "<input type=\"hidden\" name=\"merge\" value=\"" +
                beginning + "," + end + "," + segType + "\" />";
    },
    insertedBlock: function (guid, elem) {
        return "<div id=\"chosen_" + guid + "\" style=\"padding: 3px; margin-bottom: 10px\">" +
               HtmlBlocks.currentDescription(elem) + "<br />" +
               HtmlBlocks.annotateMark(guid) + "</div>";
    },
};
