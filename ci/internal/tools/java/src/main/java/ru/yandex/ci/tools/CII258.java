package ru.yandex.ci.tools;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.oldci.CiCheck;
import ru.yandex.ci.client.oldci.OldCiClient;

@SuppressWarnings("DefaultCharset")
public class CII258 {
    public static void main(String[] args) throws Exception {
        OldCiClient oldCiClient = OldCiClient.create(HttpClientProperties.ofEndpoint(Misc.OLD_CI_API_URL));

        String baseDir = "/Users/andreevdm/tmp/restart";


        BufferedReader reader = new BufferedReader(new FileReader(new File(baseDir, "builds3")));


        List<String> checks = new ArrayList<>();
        String line;
        while ((line = reader.readLine()) != null) {
            if (!line.equals("name")) {
                checks.add(line);
            }
        }

        FileWriter restartWriter = new FileWriter(new File(baseDir, "forRestart5"), true);
        FileWriter needStopWriter = new FileWriter(new File(baseDir, "needStop3"), true);


        for (String checkId : checks) {
            CiCheck check = oldCiClient.getCheck(checkId);
            if (check.getStatistics().getConfigure().getBroken() > 0) {
                System.out.println("bad check :" + checkId);
                if (check.getFinishTimestamp() <= 0) {
                    System.out.println("Need stop :" + checkId);

                    needStopWriter.write(checkId + "\n");
                    needStopWriter.flush();


                }


                restartWriter.write(checkId + "\n");
                restartWriter.flush();
            } else {
                System.out.println("good check :" + checkId);

            }


        }


        System.exit(0);
    }
}
