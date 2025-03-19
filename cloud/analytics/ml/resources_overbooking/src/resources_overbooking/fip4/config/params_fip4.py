trained_params = {
    "Cloud Service": {
        "active": {
            "paid": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "service": {
                "date_from": "2019-01-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "trial": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            }
        },
        "payment_required": {
            "paid": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "service": {
                "date_from": "2020-01-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "trial": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            }
        },
        "suspended": {
            "paid": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            }
        }
    },
    "Enterprise": {
        "active": {
            "paid": {
                "date_from": "2019-06-01",
                "forecast_days": 730,
                "forecast_type": "advanced",
                "history_days": 730,
                "params": {
                    "D": 1,
                    "P": 0,
                    "Q": 1,
                    "S": 7,
                    "boxcox": 0.32406075508039994,
                    "d": 1,
                    "p": 1,
                    "q": 1,
                    "trend": "c"
                }
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "2019-01-01",
                "forecast_days": 730,
                "forecast_type": "simple",
                "history_days": 730
            }
        },
        "payment_required": {
            "paid": {
                "date_from": "2020-01-01",
                "forecast_days": 730,
                "forecast_type": "simple",
                "history_days": 730
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            }
        },
        "suspended": {
            "paid": {
                "date_from": "2020-02-14",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "2019-01-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            }
        }
    },
    "Mass": {
        "active": {
            "paid": {
                "date_from": "2019-01-01",
                "forecast_days": 730,
                "forecast_type": "advanced",
                "history_days": 730,
                "params": {
                    "D": 1,
                    "P": 1,
                    "Q": 1,
                    "S": 7,
                    "boxcox": 0.3217951567709462,
                    "d": 1,
                    "p": 1,
                    "q": 2,
                    "trend": "c"
                }
            },
            "service": {
                "date_from": "2019-05-01",
                "forecast_days": 730,
                "forecast_type": "advanced",
                "history_days": 730,
                "params": {
                    "D": 1,
                    "P": 1,
                    "Q": 1,
                    "S": 7,
                    "boxcox": 0.12315545369582899,
                    "d": 1,
                    "p": 1,
                    "q": 1,
                    "trend": "c"
                }
            },
            "trial": {
                "date_from": "2020-01-01",
                "forecast_days": 730,
                "forecast_type": "advanced",
                "history_days": 730,
                "params": {
                    "D": 1,
                    "P": 0,
                    "Q": 1,
                    "S": 7,
                    "boxcox": -0.3478875989313713,
                    "d": 1,
                    "p": 3,
                    "q": 0,
                    "trend": "t"
                }
            }
        },
        "payment_required": {
            "paid": {
                "date_from": "2020-01-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "service": {
                "date_from": "2020-01-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "trial": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            }
        },
        "suspended": {
            "paid": {
                "date_from": "2020-09-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "service": {
                "date_from": "2020-09-07",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "trial": {
                "date_from": "2020-07-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            }
        }
    },
    "Medium": {
        "active": {
            "paid": {
                "date_from": "2020-01-01",
                "forecast_days": 730,
                "forecast_type": "simple",
                "history_days": 365
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "2019-01-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            }
        },
        "payment_required": {
            "paid": {
                "date_from": "2020-05-01",
                "forecast_days": 730,
                "forecast_type": "simple",
                "history_days": 730
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            }
        },
        "suspended": {
            "paid": {
                "date_from": "2019-10-16",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "2019-03-03",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            }
        }
    },
    "Public sector": {
        "active": {
            "paid": {
                "date_from": "2021-07-01",
                "forecast_days": 730,
                "forecast_type": "simple",
                "history_days": 365
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "2021-07-15",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            }
        },
        "payment_required": {
            "paid": {
                "date_from": "2021-07-01",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            }
        },
        "suspended": {
            "paid": {
                "date_from": "2021-09-22",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            },
            "service": {
                "date_from": "",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 730
            },
            "trial": {
                "date_from": "2021-07-10",
                "forecast_days": 730,
                "forecast_type": "const",
                "history_days": 365
            }
        }
    }
}
