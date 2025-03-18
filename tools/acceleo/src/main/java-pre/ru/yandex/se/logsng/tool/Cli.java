package ru.yandex.se.logsng.tool;

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveOutputStream;
import org.eclipse.emf.common.util.URI;

import java.io.*;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Created by akastornov on 20.02.16.
 */
public class Cli {

    private static class Parameters {
        @Parameter(names = "--xsd")
        public List<File> xsds = new ArrayList<>();

        @Parameter(names = "--mtl")
        public List<File> mtls = new ArrayList<>();

        @Parameter(names = "--lang")
        public List<File> langs = new ArrayList<>();

        @Parameter(names = "--output-dir")
        public File resultDir;

        @Parameter(names = "--build-root")
        public File buildRoot;

        @Parameter(names = "--source-root")
        public File sourceRoot;

        @Parameter(names = "--mtl-root")
        public String mtlRoot;
    }

    public static void getFilesRecursive(List<Path> files, Path dir) {
        try (DirectoryStream<Path> stream = Files.newDirectoryStream(dir)) {
            for (Path path : stream) {
                if (path.toFile().isDirectory()) {
                    getFilesRecursive(files, path);

                } else {
                    files.add(path);
                }
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static List<Path> getFilesRecursive(Path dir) {
        List<Path> files = new ArrayList<>();
        getFilesRecursive(files, dir);
        return files;
    }

    public static void copyStream(InputStream in, OutputStream out) {
        try {
            int count;
            byte data[] = new byte[2048];
            while ((count = in.read(data)) != -1) {
                out.write(data, 0, count);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static void createTarFromDir(Path dir, Path tar) {
        List<Path> paths = getFilesRecursive(dir);

        try (FileOutputStream ostream = new FileOutputStream(tar.toFile())) {
            TarArchiveOutputStream out = new TarArchiveOutputStream(new BufferedOutputStream(ostream));
            out.setLongFileMode(TarArchiveOutputStream.LONGFILE_POSIX);

            for (Path path : paths) {
                TarArchiveEntry entry = new TarArchiveEntry(path.toFile(), dir.relativize(path).toString());

                out.putArchiveEntry(entry);

                try (BufferedInputStream origin = new BufferedInputStream(new FileInputStream(path.toFile()))) {
                    copyStream(origin, out);

                    out.closeArchiveEntry();
                    out.flush();
                }

            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static void main(String[] args) {
        Parameters params = new Parameters();
        new JCommander(params, args);

        File modelFile = new File(params.resultDir, "LoggingData.ecore");

        // 1st step
        Xsd2EResource.convert(
                params.xsds
                        .stream()
                        .map(File::getAbsolutePath)
                        .collect(Collectors.toList())
                        .toArray(new String[params.xsds.size()]),
                modelFile.getAbsolutePath()
        );

        final Path srcdir = Paths.get(params.sourceRoot.getAbsolutePath()).resolve(params.mtlRoot);
        final Path bindir = Paths.get(params.buildRoot.getAbsolutePath()).resolve(params.mtlRoot);
        final Path selected = Paths.get(new File(params.resultDir, "selected").getAbsolutePath());
        final Path template = Paths.get(new File(params.resultDir, "template").getAbsolutePath());

        // Copy all mtl files into a separate directory
        for (File mtl: params.mtls) {
            final Path src = Paths.get(mtl.getAbsolutePath());
            final Path dest;

            if (src.startsWith(srcdir)) {
                dest = selected.resolve(srcdir.relativize(src));
            } else if (src.startsWith(bindir)) {
                dest = selected.resolve(bindir.relativize(src));
            } else {
                throw new RuntimeException("MTL file " + src + " must be child of either " + srcdir + " or " + bindir);
            }

            dest.getParent().toFile().mkdirs();

            try {
                Files.copy(src, dest);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        // 2nd step
        AcceleoCompiler.compile(selected.toFile().getAbsolutePath(), template.toFile().getAbsolutePath(), "");

        // 3rd step
        for (File lang: params.langs) {
            try {
                CodeGenerator.generate(
                        params.resultDir.getAbsolutePath(),
                        "template",
                        URI.createFileURI(modelFile.getAbsolutePath()),
                        new FileInputStream(lang)
                );
            } catch (FileNotFoundException e) {
                throw new RuntimeException(e);
            }
        }

        // 4th step
        createTarFromDir(
                Paths.get(params.resultDir.getAbsolutePath()),
                Paths.get(new File(params.resultDir, "generated.tar").getAbsolutePath())
        );
    }
}
