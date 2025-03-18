// this is example PGreed mock

window.PGreed = {
    get: function () {
        return new Promise((resolve, reject) => {
            setTimeout(function () {
                resolve({
                    "a1":"Fcv81w==;0",
                    "a2":"JUE2ZTr2aL4g5OTak9+5g8qLk0/n/A==;1",
                    "a3":"1r5NsIG9youO9/QgwoxuNg==;2",
                    "x5":"sZyK9KNeJkAI6yMW;99",
                    "v":"6.3.1",
                    "pgrdt":"5avhxMhqMGirwBgufRmwLkqk440=;100",
                    "pgrd":"rvj3xtj7c4SLtZwpqioqGsq30hhE7rACdrB4spjj/Qh2qClFgaBIwMQcy7B4+z+wI5Gt2i/Gm2dzbz4dQoY6V9ZNK0fLg8RDX5dn4CTjEH2UzOf9jem/6JcGPscRvVdVtbEnw5Q6AmHBtaTwWP8yBUs/0FJEYzLj6F2sA5E6s3i4el/U4f1mnz/tKFdGe3KroHtLAyu05UeNc2kveEnrGfGKG3U="
                });
            }, 300);
        });
    }
};