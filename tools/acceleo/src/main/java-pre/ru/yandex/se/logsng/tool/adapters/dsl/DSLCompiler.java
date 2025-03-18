package ru.yandex.se.logsng.tool.adapters.dsl;

import ru.yandex.se.logsng.tool.adapters.dsl.statements.ObjStatement;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;

/**
 * Created by astelmak on 24.05.16.
 */
public class DSLCompiler {
    private static File getTempDir() throws IOException {
        File tmpFile = File.createTempFile("scarab", "java");
        File tmpDir = tmpFile.getParentFile();

        //noinspection ResultOfMethodCallIgnored
        tmpFile.delete();

        File result = new File(tmpDir, "scarab_dsl_codegen_" + System.getProperty("user.name"));

        //noinspection ResultOfMethodCallIgnored
        result.mkdir();

        return result;
    }

    private static String getClassPath() {
        ClassLoader cl = DSLCompiler.class.getClassLoader();

        URL[] urls = ((URLClassLoader)cl).getURLs();

        StringBuilder b = new StringBuilder();

        boolean sep = false;
        for(URL url: urls) {
            if (sep)
                b.append(File.pathSeparator);
            else
                sep = true;
            b.append(url.getFile());
        }
        return b.toString();
    }

    private static class ClassLoaderData {
        final String dir;
        final ClassLoader loader;

        ClassLoaderData(String dir, ClassLoader loader) {
            this.dir = dir;
            this.loader = loader;
        }
    }

    private static volatile ClassLoaderData clData;
    private static ClassLoader getClassloader(File dir) throws MalformedURLException {
        ClassLoaderData cld = clData;

        if (cld == null || !clData.dir.equals(dir.getAbsolutePath())) {
            synchronized (DSLCompiler.class) {
                cld = clData;

                if (cld == null || !clData.dir.equals(dir.getAbsolutePath())) {
                    //noinspection deprecation
                    clData = cld = new ClassLoaderData(dir.getAbsolutePath(),
                            new URLClassLoader(new URL[] { dir.toURL() }, DSL.class.getClassLoader()));
                }
            }
        }

        return cld.loader;
    }

    private static DSL compile(String className, File srcFile) throws Exception {
        String classPath = getClassPath();
        File classesDir = srcFile.getParentFile();

        int errorCode = com.sun.tools.javac.Main.compile(new String[] {
                "-classpath", classPath,
                "-d", classesDir.toString(),
                srcFile.toString() }, new PrintWriter(System.err));

        if (errorCode != 0)
            throw new Exception("Error compile class: " + className + ", ErrorCode=" + errorCode);

        Class cls = null;
        ClassLoader loader = getClassloader(classesDir);
        try {
            cls = loader.loadClass(className);
        }
        catch(ClassNotFoundException t) {
            // dirty hack - wait for file SYNC!!!!!!
            for (int i = 0; i < 50; i++) {
                Thread.sleep(200);
                try {
                    cls = loader.loadClass(className);
                    break;
                }
                catch(ClassNotFoundException t1){}
            }
        }

        if (cls == null)
            cls = loader.loadClass(className);

        return (DSL)cls.newInstance();
    }

    public static DSL compile(String exp, String className, Class compilerImpl) throws Exception {
        File srcFile = new File(getTempDir(), className + ".java");
        File classFile = new File(getTempDir(), className + ".class");

        if (srcFile.exists() && !srcFile.delete())
            throw new Exception("Can't delete temporary source file: " + srcFile);

        if (classFile.exists() && !classFile.delete())
            throw new Exception("Can't delete temporary class file: " + classFile);

        try (PrintWriter out = new PrintWriter(new FileOutputStream(srcFile))) {
            out.println("import ru.yandex.se.logsng.tool.adapters.dsl.*;");
            out.println("import ru.yandex.se.logsng.tool.adapters.dsl.statements.*;\n");

            out.println("public class " + className + " extends " + compilerImpl.getName() + " {");
            out.println("    public Statement evalImpl(" + ObjStatement.class.getName() + " event) {");
            out.println("       return (" + exp + ");");
            out.println("    }");
            out.println("}");

            out.flush();
        }

        return compile(className, srcFile);
    }
}
