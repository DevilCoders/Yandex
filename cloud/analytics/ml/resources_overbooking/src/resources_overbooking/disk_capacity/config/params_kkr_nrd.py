trained_params = {
    "preprod": {
        "hdd": {
            "all": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-08-05",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 4,
                    "q": 5,
                    "trend": "c"
                }
            },
            "myt": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-07-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 5,
                    "q": 3,
                    "trend": "c"
                }
            },
            "sas": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-07-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 4,
                    "q": 5,
                    "trend": "c"
                }
            },
            "vla": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-08-05",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 1,
                    "trend": "c"
                }
            }
        },
        "ssd": {
            "all": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-07-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 3,
                    "q": 3,
                    "trend": "c"
                }
            },
            "myt": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-07-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 2,
                    "q": 5,
                    "trend": "c"
                }
            },
            "sas": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-07-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 2,
                    "trend": "c"
                }
            },
            "vla": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-10-15",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 0,
                    "trend": "c"
                }
            }
        }
    },
    "prod": {
        "hdd": {
            "all": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-06-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 1,
                    "trend": "t"
                }
            },
            "myt": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-06-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 1,
                    "trend": "c"
                }
            },
            "sas": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-06-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 1,
                    "trend": "t"
                }
            },
            "vla": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-06-01",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 1,
                    "trend": "c"
                }
            }
        },
        "ssd": {
            "all": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-04-20",
                "periods_to_ignore": [
                    {
                        "comment": "Касперский +200ТБ",
                        "from_date": "2021-01-18",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация (VLA)",
                        "from_date": "2021-02-01",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация",
                        "from_date": "2021-05-18",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация-2",
                        "from_date": "2021-10-02",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация-3",
                        "from_date": "2021-11-27",
                        "num_days": 14
                    },
                    {
                        "comment": "Дефрагментация-4",
                        "from_date": "2022-01-16",
                        "num_days": 14
                    },
                    {
                        "comment": "",
                        "from_date": "2022-03-23",
                        "num_days": 7
                    },
                    {
                        "comment": "",
                        "from_date": "2022-04-05",
                        "num_days": 7
                    },
                    {
                        "comment": "",
                        "from_date": "2022-04-14",
                        "num_days": 7
                    }
                ],
                "sarimax": {
                    "D": 1,
                    "P": 1,
                    "Q": 1,
                    "S": 7,
                    "d": 1,
                    "p": 4,
                    "q": 4,
                    "trend": "c"
                }
            },
            "myt": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-04-15",
                "periods_to_ignore": [
                    {
                        "comment": "Дефрагментация",
                        "from_date": "2021-05-18",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация-2",
                        "from_date": "2021-10-20",
                        "num_days": 7
                    }
                ],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 1,
                    "q": 1,
                    "trend": "c"
                }
            },
            "sas": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-04-15",
                "periods_to_ignore": [
                    {
                        "comment": "Дефрагментация",
                        "from_date": "2021-05-18",
                        "num_days": 7
                    },
                    {
                        "comment": "",
                        "from_date": "2022-03-10",
                        "num_days": 14
                    },
                    {
                        "comment": "",
                        "from_date": "2022-03-29",
                        "num_days": 7
                    }
                ],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 2,
                    "trend": "c"
                }
            },
            "vla": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2020-04-20",
                "periods_to_ignore": [
                    {
                        "comment": "Касперский +200ТБ",
                        "from_date": "2021-01-18",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация (VLA)",
                        "from_date": "2021-02-01",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация",
                        "from_date": "2021-05-18",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация-2",
                        "from_date": "2021-10-20",
                        "num_days": 7
                    },
                    {
                        "comment": "Дефрагментация-3",
                        "from_date": "2021-11-27",
                        "num_days": 14
                    },
                    {
                        "comment": "Дефрагментация-4",
                        "from_date": "2022-01-16",
                        "num_days": 14
                    },
                    {
                        "comment": "",
                        "from_date": "2022-02-14",
                        "num_days": 7
                    },
                    {
                        "comment": "",
                        "from_date": "2022-03-22",
                        "num_days": 7
                    },
                    {
                        "comment": "",
                        "from_date": "2022-04-07",
                        "num_days": 7
                    }
                ],
                "sarimax": {
                    "D": 1,
                    "P": 1,
                    "Q": 1,
                    "S": 7,
                    "d": 1,
                    "p": 4,
                    "q": 4,
                    "trend": "c"
                }
            }
        },
        "ssd_nrd": {
            "all": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2021-03-10",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 2,
                    "q": 2,
                    "trend": "t"
                }
            },
            "myt": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2021-03-10",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 1,
                    "q": 1,
                    "trend": "t"
                }
            },
            "sas": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2021-03-10",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 1,
                    "q": 1,
                    "trend": "c"
                }
            },
            "vla": {
                "days_in_history": 365,
                "days_to_forecast": 730,
                "from_date": "2021-03-10",
                "periods_to_ignore": [],
                "sarimax": {
                    "D": 0,
                    "P": 0,
                    "Q": 0,
                    "S": 0,
                    "d": 1,
                    "p": 0,
                    "q": 2,
                    "trend": "c"
                }
            }
        }
    }
}
