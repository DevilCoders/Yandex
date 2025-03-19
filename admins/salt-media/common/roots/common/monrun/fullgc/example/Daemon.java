import java.util.logging.*;
import java.io.*;
import java.lang.ProcessBuilder;
import java.lang.management.ManagementFactory;


public class Daemon extends Thread
{
  private static transient Logger log = Logger.getLogger("DaemonLogger");

  public static void main(String args[]) throws IOException {
    FileHandler fh;

    if (args.length < 1) {
        System.err.println ("Usage: Daemon [log dir]");
        log.info("Usage: Daemon [log dir]");
        return;
    }
    if (args.length > 1) {
      System.err.println ("Args > 1");
      for (int i=0; i < Integer.parseInt(args[1]); i++){
        System.err.println ("Start new process");
        ProcessBuilder p = new ProcessBuilder();
        p.command("java", "Daemon", args[0].toString());
        p.start();
      }
    } else {
      try {
        System.err.println ("Initialize process");
        String fname = ManagementFactory.getRuntimeMXBean().getName().replaceAll("@.*", "");
        fh = new FileHandler(args[0].concat("/").concat(fname).concat("-gc.log"));
        log.addHandler(fh);
        SimpleFormatter formatter = new SimpleFormatter();
        fh.setFormatter(formatter);
        log.setUseParentHandlers(false);
        log.info("Logger initialized");

        while(true){
          log.info("Sample log about Full GC in sample daemon");
          Thread.sleep(5000);
        }
      } catch (Exception e){
        System.err.println("Exception occurred:" + e.toString());
        e.printStackTrace();
      }
    }
  }
}
