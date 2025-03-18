JAVA_PROGRAM()

JDK_VERSION(11)

SET(jackson.version 2.4.2)
SET(trove.version 3.0.3)

NO_COMPILER_WARNINGS()

OWNER(
    g:scarab
    akastornov
)

PEERDIR(
    # option parser
    contrib/java/com/beust/jcommander

    contrib/java/com/google/guava/guava/18.0
    contrib/java/com/fasterxml/jackson/core/jackson-databind/${jackson.version}
    contrib/java/org/eclipse/acceleo/org.eclipse.acceleo.engine/3.6.2-20160115.071807-26
    contrib/java/org/eclipse/acceleo/org.eclipse.acceleo.parser/3.6.2-20160115.071802-26
    contrib/java/org/eclipse/acceleo/org.eclipse.acceleo.model/3.6.2-20160115.071739-26
    contrib/java/org/eclipse/acceleo/org.eclipse.acceleo.common/3.6.2-20160115.071729-26
    contrib/java/org/eclipse/acceleo/org.eclipse.acceleo.profiler/3.6.2-20160115.071745-26
    contrib/java/lpg/runtime/java/lpg.runtime.java/2.0.17.v201004271640

    contrib/java/org/apache/commons/commons-lang3/3.4

    contrib/java/org/codehaus/groovy/groovy-all/2.1.8  # provided
    contrib/java/org/eclipse/core/runtime/3.10.0-v20140318-2214
    contrib/java/org/eclipse/core/resources/3.3.0-v20070604
    contrib/java/org/eclipse/xsd/org.eclipse.xsd/2.11.0-v20150806-0404

    contrib/java/org/eclipse/emf/org.eclipse.emf.ecore/2.9.1.v20130827-0309
    contrib/java/org/eclipse/emf/org.eclipse.emf.ecore.xmi/2.9.1.v20130827-0309
    contrib/java/org/eclipse/emf/org.eclipse.emf.common/2.9.1.v20130827-0309

    contrib/java/org/eclipse/ocl/org.eclipse.ocl/3.3.0.v20130909-1552
    contrib/java/org/eclipse/ocl/org.eclipse.ocl.common/1.1.0.v20130531-0544
    contrib/java/org/eclipse/ocl/org.eclipse.ocl.ecore/3.3.0.v20130520-1222

    contrib/java/net/sf/trove4j/trove4j/${trove.version}
    contrib/java/commons-cli/commons-cli/1.3.1
    contrib/java/com/github/mifmif/generex/0.0.4

    contrib/java/org/hamcrest/hamcrest-core/1.3
    contrib/java/org/apache/commons/commons-compress/1.9
)

IF (JDK_VERSION == "8")
    PEERDIR(contrib/java/com/sun/tools/1.8.0_60)
ENDIF()

JAVA_SRCS(SRCDIR src/main/java-pre **/*.java)
NO_LINT()

# Added automatically to remove dependency on default contrib versions
DEPENDENCY_MANAGEMENT(
    contrib/java/com/beust/jcommander/1.78
)

END()
