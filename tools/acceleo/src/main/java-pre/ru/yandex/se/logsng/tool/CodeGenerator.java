package ru.yandex.se.logsng.tool;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonFactory;
import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.eclipse.acceleo.common.AcceleoServicesRegistry;
import org.eclipse.acceleo.engine.service.AbstractAcceleoGenerator;
import org.eclipse.emf.common.notify.Adapter;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.util.*;
import org.eclipse.emf.ecore.*;
import org.eclipse.emf.ecore.resource.Resource;

import java.io.*;
import java.lang.reflect.InvocationTargetException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.*;
import java.util.concurrent.Callable;

/**
 *
 * Created by minamoto on 03/09/15.
 */

class Counter {
  final String name;
  final String limitName;
  @JsonCreator
  public Counter(@JsonProperty("name") final String name,
                 @JsonProperty("limit-name") final String limitName){
    this.name = name;
    this.limitName = limitName;
  }
}

class ConfigParameter {
  final String name;

  ConfigParameter(String name) {
    this.name = name;
  }
}

class Variable extends ConfigParameter {
  final String value;
  @JsonCreator
  Variable(@JsonProperty("name") final String name, @JsonProperty("value") final String value) {
    super(name);
    this.value = value;
  }
}

class LanguageStateConfiguration {
  final String name;
  final Counter dependencyCounter;
  final Counter inheritanceCounter;
  @JsonCreator
  LanguageStateConfiguration(@JsonProperty ("name") final String name,
                             @JsonProperty ("dependency-counter") final Counter dependencyCounter,
                             @JsonProperty ("inheritance-counter") final Counter inheritanceCounter) {
    this.name = name;
    this.dependencyCounter = dependencyCounter;
    this.inheritanceCounter = inheritanceCounter;
  }
}

class LanguageGenerationConfiguration {
  final String languageName;
  final Collection<LanguageStateConfiguration> stages;
  final Collection<Variable> vars;
  final Collection<String> features;
  @JsonCreator
  public LanguageGenerationConfiguration(@JsonProperty("name") final String languageName,
                                         @JsonProperty("stages") final Collection<LanguageStateConfiguration> stages,
                                         @JsonProperty("vars") final Collection<Variable> vars,
                                         @JsonProperty("features") final Collection<String> features) {
    this.languageName = languageName;
    this.stages = stages;
    this.vars = vars != null ? vars : Collections.EMPTY_LIST;
    this.features = features != null ? features : Collections.EMPTY_LIST;
  }
}


public class CodeGenerator {
  final static ObjectMapper MAPPER = new ObjectMapper(new JsonFactory() {
    @Override
    public JsonParser createParser(InputStream in) throws IOException, JsonParseException {
      JsonParser parser = super.createParser(in);
      parser.enable(JsonParser.Feature.ALLOW_COMMENTS);
      return parser;
    }
  });
  static {
    MAPPER.configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);
  }

  static private  HashSet<String> globalFeatures = new HashSet<>();
  static private HashMap<String, String> globalVars = new HashMap<>();

  public static void generate(final String buildDir, final String templateHome, final URI moduleUri, InputStream in) {
    System.err.println("CodeGenerator.generate(" + buildDir +", " + templateHome + " URI(" + moduleUri + ")), stream");
    try {
      final File generationTargetFolder = new File(buildDir);
      final LanguageGenerationConfiguration configuration = MAPPER.readValue(in, LanguageGenerationConfiguration.class);

      configuration.vars.stream().forEach(x -> globalVars.put(x.name, x.value));
      configuration.features.stream().forEach(x -> globalFeatures.add(x));

      globalVars.put("__lang__", configuration.languageName);
      for (final LanguageStateConfiguration config:configuration.stages) {
        Callable<AbstractAcceleoGenerator> f = new Callable() {
          public AbstractAcceleoGenerator call() throws Exception {
            try {
              return createGenerator(buildDir, templateHome, moduleUri, generationTargetFolder, configuration, config);
            }
            catch (IOException e) {
              throw new RuntimeException(e);
            }
          }
        };

        doGeneration(f, config);

      }
    }
    catch (Exception e) {
      throw new RuntimeException(e);
    }
  }

  public static Boolean feature(String varName) {
    return globalFeatures.contains(varName);
  }

  public static String resolve(String varName) {
    final String value = globalVars.get(varName);
    if (value == null)
      throw new NullPointerException("no variable " + varName);
    return value;
  }

  public static String resolve(String varName, String defValue) {
    final String value = globalVars.get(varName);
    return value == null ? defValue : value;
  }

  public static String getEnv(String varName, String defValue) {
    final String value = System.getenv(varName);
    return value == null ? (defValue == null ? varName : defValue) : value;
  }

  public static String getEnv(String varName) {
    return getEnv(varName, null);
  }

  private static AbstractAcceleoGenerator createGenerator(final String buildDir, final String templateHome, URI moduleUri, final File generationTargetFolder, final LanguageGenerationConfiguration configuration, final LanguageStateConfiguration config) throws IOException {
    final String[] templateDirs = templateHome.split(":");
    AbstractAcceleoGenerator generator = new AbstractAcceleoGenerator() {

      EObject proxyModel = null;
      @Override
      public EObject getModel() {
        if (proxyModel == null) {
          final EList<EObject> list = new ClassifierCollections.EListImpl<>();
          final ArrayList<EObject> objects = new ArrayList<>();
          final EObject model = super.getModel();
          objects.addAll(model.eContents());
          final ArrayList<EObject> unresolved = new ArrayList<>();
          unresolved.addAll(objects);
          final ArrayList<EObject> resolved = new ArrayList<>();
          while (!unresolved.isEmpty()) {
            for(EObject o:unresolved) {
              if (!(o instanceof EClassifier)
                  || o instanceof EDataType) {
                resolved.add(o);
                continue;
              }

              if (o instanceof EClass) {
                EClass clazz = (EClass) o;
                boolean parentResolved = clazz.getEAllSuperTypes().isEmpty() || resolved.contains(clazz.getEAllSuperTypes().get(clazz.getEAllSuperTypes().size() - 1));

                if (!parentResolved)
                  continue;
                boolean childrenResolved = true;
                EList<EStructuralFeature> structuralFeatures = clazz.getEAllStructuralFeatures();
                for (EStructuralFeature sf:structuralFeatures) {
                  EClassifier classifier = sf.getEType();
                  childrenResolved =  (classifier instanceof EDataType) || resolved.contains(sf.getEType());
                  if (!childrenResolved)
                    break;
                }

                if (childrenResolved) {
                  resolved.add(o);
                }
              }
            }

            unresolved.removeAll(resolved);
          }
          list.addAll(resolved);
          proxyModel = new ProxyModel(model, list);
        }
        return proxyModel;
      }

      @Override
      public String getModuleName() {
        return config.name;
      }

      @Override
      public String[] getTemplateNames() {
        return new String[]{config.name};
      }

      @Override
      public File getTargetFolder() {
        System.err.println("getTargetFor: " + generationTargetFolder);
        return generationTargetFolder;
      }

      @Override
      public URL findModuleURL(final String moduleName) {
        try {

          for (String templateDir:templateDirs) {
            File template = new File(buildDir + "/" + templateDir + "/" + config.name + ".emtl");
            boolean exists = template.exists();
            System.err.println("trying " + template + " ... " + exists);
            if (!exists) {
              template = new File(buildDir + "/" + templateDir + "/" + configuration.languageName + "/" + config.name + ".emtl");
              exists = template.exists();
              System.err.println("trying " + template + " ... " + exists);
              if (!exists) continue;
            }
            URL url = template.toURL();
            System.err.println("findModuleURL: " + url.toString());
            return url;
          }
          throw new RuntimeException("not found " + config.name + ".emtl");
        } catch (MalformedURLException e) {
          throw new RuntimeException(e);
        }
      }
    };
    generator.initialize(moduleUri, generationTargetFolder, Collections.EMPTY_LIST);
    return generator;
  }

  private static void doGeneration(Callable<AbstractAcceleoGenerator> f, LanguageStateConfiguration config) {
    if (config.inheritanceCounter != null && config.dependencyCounter != null)
      throw new RuntimeException("invalid language configuration");
    if (config.inheritanceCounter != null)
      doGenerationWithInheritanceCounter(f, config.inheritanceCounter);
    else if (config.dependencyCounter != null)
      doGenerationDependencyLimitedCounter(f, config.dependencyCounter);
    else
      doGeneration(f);
    AcceleoServicesRegistry.INSTANCE.clearRegistry();
  }

  private static void doGeneration(Callable<AbstractAcceleoGenerator> f) {
    try {
      f.call().generate(new BasicMonitor());
    }
    catch (Exception e) {
      throw new RuntimeException(e);
    }
  }

  private static void doGenerationDependencyLimitedCounter(Callable<AbstractAcceleoGenerator> f, Counter dependencyCounter) {
    int limit = CounterUtils.value(dependencyCounter.limitName);
    while (CounterUtils.value(dependencyCounter.name) != limit) {
      doGeneration(f);
    }
  }

  private static void doGenerationWithInheritanceCounter(Callable<AbstractAcceleoGenerator> f, Counter inheritanceCounter) {
    for (int i = 0, limit = CounterUtils.value(inheritanceCounter.limitName); i != limit + 1; ++i) {
      doGeneration(f);
      CounterUtils.inc(inheritanceCounter.name); /* note: here we increment the counter */
    }
  }

  public static void main(String[] arg) throws Exception {
    //CodeGenerator.generate(/Users/minamoto/ws/scarab/target/generated-sources/,
    // template/common:template:lang,
    // URI(file:/Users/minamoto/ws/scarab/target/generated-sources/LoggingData.ecore))

    ///Users/minamoto/ws/scarab/target/generated-sources/,
    // /Users/minamoto/ws/scarab/target/generated-sources//selected/template/common:template:lang
    // URI(file:/Users/minamoto/ws/scarab/target/generated-sources/LoggingData.ecore)
    generate("/Users/minamoto/ws/scarab/target/generated-sources/",
        "target/generated-sources//selected/template/common:template:lang",
        URI.createURI("file:/Users/minamoto/ws/scarab/target/generated-sources/LoggingData.ecore"),
        new FileInputStream(new File("/Users/minamoto/ws/scarab/lang/stats.json")));

    generate("/Users/minamoto/ws/scarab/target/generated-sources/",
        "target/generated-sources//selected/template/common:template:lang",
        URI.createURI("file:/Users/minamoto/ws/scarab/target/generated-sources/LoggingData.ecore"),
        new FileInputStream(new File("/Users/minamoto/ws/scarab/lang/java-io2.json")));

    //generate("target", new FileInputStream(new File("/Users/minamoto/ws/scarab/./log-api/template/stats.json")));
  }

  private static class ProxyModel implements EObject {
    private final EObject model;
    private final EList<EObject> list;

    public ProxyModel(EObject model, EList<EObject> list) {
      this.model = model;
      this.list = list;
    }

    @Override
    public EClass eClass() {
      return model.eClass();
    }

    @Override
    public Resource eResource() {
      return model.eResource();
    }

    @Override
    public EObject eContainer() {
      return model.eContainer();
    }

    @Override
    public EStructuralFeature eContainingFeature() {
      return model.eContainingFeature();
    }

    @Override
    public EReference eContainmentFeature() {
      return model.eContainmentFeature();
    }

    @Override
    public EList<EObject> eContents() {
      return list;
    }

    @Override
    public TreeIterator<EObject> eAllContents() {
      return new AbstractTreeIterator(this, false) {
        private static final long serialVersionUID = 1L;

        public Iterator<EObject> getChildren(Object object) {
          return ((EObject)object).eContents().iterator();
        }
      };

//      return model.eAllContents();
    }

    @Override
    public boolean eIsProxy() {
      return model.eIsProxy();
    }

    @Override
    public EList<EObject> eCrossReferences() {
      return model.eCrossReferences();
    }

    @Override
    public Object eGet(EStructuralFeature eStructuralFeature) {
      return model.eGet(eStructuralFeature);
    }

    @Override
    public Object eGet(EStructuralFeature eStructuralFeature, boolean b) {
      return model.eGet(eStructuralFeature, b);
    }

    @Override
    public void eSet(EStructuralFeature eStructuralFeature, Object o) {
      model.eSet(eStructuralFeature, o);
    }

    @Override
    public boolean eIsSet(EStructuralFeature eStructuralFeature) {
      return eIsSet(eStructuralFeature);
    }

    @Override
    public void eUnset(EStructuralFeature eStructuralFeature) {
      model.eUnset(eStructuralFeature);
    }

    @Override
    public Object eInvoke(EOperation eOperation, EList<?> eList) throws InvocationTargetException {
      return model.eInvoke(eOperation, eList);
    }

    @Override
    public EList<Adapter> eAdapters() {
      return model.eAdapters();
    }

    @Override
    public boolean eDeliver() {
      return model.eDeliver();
    }

    @Override
    public void eSetDeliver(boolean b) {
      model.eSetDeliver(b);
    }

    @Override
    public void eNotify(Notification notification) {
      model.eNotify(notification);
    }
  }
}
