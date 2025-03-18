{
    "Name": "user scheme",
    "Comment": "Example of scheme usage. this level describes index upper level",
    "Atlas": "checked_continuous",

    "Child0": {
        "Name": "PerDocData",
        "Atlas": "checked_continuous",
        "Children": {
            "Name": "SingleDocData",
            "Atlas": "checked_continuous",
            "Child0": {
                "Name": "Title",
                "Codec": "None",
                "DbgViewer": "utf8_printer"
            },
            "Child1": {
                "Name": "Url",
                "Codec": "ComptableHQ",
                "DbgViewer": "utf8_printer"
            }
        }
    },

    "Child1": {
        "Name": "MiscInfo",
        "Atlas": "checked_continuous",
        "Child0": {
            "Name": "CreationDate",
            "Codec": "None"
        },
        "Child1": {
            "Name": "CreationDir",
            "Codec": "None",
            "DbgViewer": "utf8_printer"
        }
    }
}
