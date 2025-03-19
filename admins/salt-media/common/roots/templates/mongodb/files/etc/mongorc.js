var currDir = pwd();
var mongorc = "/etc/mongorc.d";
cd(mongorc);
var files = listFiles();
for ( i = 0; i < files.length; i++) {
    file = files[i];
    if (! file["isDirectory"] && file["name"].endsWith("js")) {
        load(file["name"]);
    }
}
cd(currDir);

