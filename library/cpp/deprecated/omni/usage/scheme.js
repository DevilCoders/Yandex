{
    "Name": "user scheme",
    "Comment": "Example of scheme usage. this level describes index upper level",
    "Atlas": "len_continuous",

    "Child0": {
        "Name": "PerDocData",
        "Atlas": "len_continuous",
        "Children": {
            "Name": "SingleDocData",
            "Atlas": "len_continuous",
            "Child0": {
                "Name": "Title",
                "Codec": "None",
                "TablePath": "precreated_tables/dummy_table1",
                "DbgViewer": "utf8_printer"
            },
            "Child1": {
                "Name": "Breaks",
                "Atlas": "len_continuous",
                "Children": {
                    "Name": "BreakText",
                    "Codec": "None",
                    "DbgViewer": "utf8_printer"
                }
            }
        }
    },

    "Child1": {
        "Name": "MiscInfo",
        "Atlas": "len_continuous",
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
