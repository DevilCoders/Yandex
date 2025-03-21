instance = {
    Name = "xxx";

    RequestTimeout = "10ms";
    request_timeout = "5s";

    InnerOptions = {
        V = 111;
        M2 = 123;
    };

    OtherOptions = {
        Deadline = "1s";
    };

    RepeatedOptions = {
        {},
        {
            Deadline = "1s";
            DefaultDeadline = "10s";
            unknown_field_r = {};
        }
    };

    Proto3Options = {
        Delay = "1s";
    };

    CustomOptions1 = "value1,value2";

    SimpleMap = {
        a = "b",
        c = "d",
    };

    SomeMap = {
        a = {
            OtherMap = {
                b = {
                    V = 222;
                }
            };
        };
        c = {
            OtherMap = {
                d = {
                    V = 333;
                }
            }
        }
    };

    SomeEnum = "ONE";

    unknown_field = {};
};
