import * as express from "express";
import fs = require("fs");
import child_process = require("child_process");
import request = require("request");

// Express initialization
let app = express();

app.set("view engine", "pug");
app.set("views", "engine/src");

app.use(express.static("engine/static"));
app.use(express.static("markup/dst"));


// Environment detection
const envFile = "/etc/yandex/environment.type";
let environment = "development";
if (fs.existsSync(envFile)) {
    environment = fs.readFileSync(envFile, { encoding: "utf8" }).replace(/\s+$/, "");
}

let sock: number|string = 8000;

const sockName = "app.sock";
if (environment === "testing") {
    sock = sockName;
    fs.unlinkSync(sockName);
}

// The index handler
app.use("/", function(req: express.Request, res: express.Response): void {
    res.render("index", { env: environment });
});

// Gogogo!
console.log(`Running ${environment} env on ${sock}`);
app.listen(sock);
