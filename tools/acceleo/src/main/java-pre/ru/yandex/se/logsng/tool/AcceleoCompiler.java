package ru.yandex.se.logsng.tool;

import gnu.trove.map.TObjectLongMap;
import gnu.trove.map.hash.TObjectLongHashMap;
import org.eclipse.acceleo.parser.compiler.AbstractAcceleoCompiler;
import org.eclipse.emf.common.util.BasicMonitor;
import org.eclipse.emf.common.util.Monitor;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EPackage;

import java.io.*;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created by minamoto on 07/09/15.
 */
public class AcceleoCompiler extends AbstractAcceleoCompiler {
  public static void compile(String sourceFolderPath, String outputFolderPath, String semicomaSeparatedDependencies) {
    AbstractAcceleoCompiler helper = new AcceleoCompiler();
    helper.setDependencies(semicomaSeparatedDependencies);
    helper.setBinaryResource(false);
    helper.setOutputFolder(outputFolderPath);
    helper.setSourceFolder(sourceFolderPath);
    helper.doCompile(new BasicMonitor.Printing(System.out));
    System.out.println("compilation finished");
  }

  public static void main(String[] arg) {
    AcceleoCompiler.compile("/Users/minamoto/ws/scarab/template", "/Users/minamoto/ws/scarab/target/generated-sources/template", "/Users/minamoto/ws/scarab/target/generated-sources/template/common");
  }

  @Override
  public void setSourceFolder(String theSourceFolder) {
    super.setSourceFolder(theSourceFolder);
  }

  @Override
  public void setDependencies(String allDependencies) {
/*    String[] deps = allDependencies.split(";");
    for (String d : deps) {
      File f = new File(d);
      if (f.exists()) {
        this.dependencies.add(f);
        this.dependenciesIDs.add(f.getName());
      }
    }
    */
    super.setDependencies(allDependencies);
  }

  @Override
  protected void registerPackages() {
    super.registerPackages();
  }

  @Override
  protected EPackage getOCLStdLibPackage() {
    return super.getOCLStdLibPackage();
  }

  @Override
  protected void registerResourceFactories() {
    super.registerResourceFactories();
  }

  @Override
  protected void registerLibraries() {
    super.registerLibraries();
  }

  @Override
  public void setBinaryResource(boolean binaryResource) {
    super.setBinaryResource(binaryResource);
  }

  @Override
  public void setOutputFolder(String theOutputFolder) {
    super.setOutputFolder(theOutputFolder);
  }

  @Override
  public void setTrimPosition(boolean trimPosition) {
    super.setTrimPosition(trimPosition);
  }

  @Override
  public void doCompile(Monitor monitor) {
    super.doCompile(monitor);
  }

  @Override
  protected void compile(Monitor monitor) {
    super.compile(monitor);
  }

  @Override
  protected List<MTLFileInfo> computeFileInfos(File theSourceFolder) {
    return super.computeFileInfos(theSourceFolder);
  }


  @Override
  protected void members(List<File> filesOutput, File container, String extension) {
    super.members(filesOutput, container, extension);
    Collections.sort(filesOutput, new Comparator<File>() {
      @Override
      public int compare(File o1, File o2) {
        try {
          return Long.compare(importsCount(o1), importsCount(o2));
        } catch (IOException e) {
          throw new RuntimeException(e);
        }
      }
    });
  }

  Pattern importPattern = Pattern.compile("\\[import\\ (.*)/\\]\\ *$");
  TObjectLongMap<File> CACHE = new TObjectLongHashMap<>();
  private long importsCount(final File f) throws IOException {
    LineNumberReader lineReader = new LineNumberReader(new FileReader(f));
    String line;
    if (!CACHE.containsKey(f)) {
      while ((line = lineReader.readLine()) != null) {
        Matcher matcher = importPattern.matcher(line);
        if (matcher != null && matcher.find(0))
          CACHE.adjustOrPutValue(f, 1, 1);
      }
    }
    return CACHE.get(f);
  }

  @Override
  protected void computeDependencies(List<URI> dependenciesURIs, Map<URI, URI> mapURIs) {
    Iterator identifiersIt = this.dependenciesIDs.iterator();
    Iterator dependenciesIt = this.dependencies.iterator();

    while (dependenciesIt.hasNext() && identifiersIt.hasNext()) {
      File requiredFolder = (File) dependenciesIt.next();
      String identifier = (String) identifiersIt.next();
      if (requiredFolder != null && requiredFolder.exists() && requiredFolder.isDirectory()) {
        String requiredFolderAbsolutePath = requiredFolder.getAbsolutePath();
        ArrayList emtlFiles = new ArrayList();
        this.members(emtlFiles, requiredFolder, "emtl");
        Iterator var10 = emtlFiles.iterator();

        while (var10.hasNext()) {
          File emtlFile = (File) var10.next();
          String emtlAbsolutePath = emtlFile.getAbsolutePath();
          URI emtlFileURI = URI.createFileURI(emtlAbsolutePath);
          dependenciesURIs.add(emtlFileURI);
          mapURIs.put(emtlFileURI, emtlFileURI);
        }
      }
    }
  }

  @Override
  protected void computeJarDependencies(List<URI> dependenciesURIs, Map<URI, URI> mapURIs) {
    super.computeJarDependencies(dependenciesURIs, mapURIs);
  }

  @Override
  protected void loadEcoreFiles() {
    super.loadEcoreFiles();
  }

  @Override
  protected void createOutputFiles(List<URI> emtlAbsoluteURIs) {
    super.createOutputFiles(emtlAbsoluteURIs);
  }
}
