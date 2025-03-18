package ru.yandex.se.logsng.tool;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClassifier;

import java.util.*;

/**
 *
 *  Created by minamoto on 13/11/15.
 */
public class Modules {
  private static final Set<String> MODULES = new HashSet<>();

  public Modules() {}

  public void registerModule(String module) {
    if (module == null)
      return;
    MODULES.add(module);
  }

  public EList<String> modules() {
    EList<String> list = new ClassifierCollections.EListImpl<>();
    list.addAll(MODULES);
    return list;
  }

  private static HashMap<String, EList<EClassifier>> module2DataTypes = new HashMap<>();
  public void registerDatatype(String module, EClassifier udt) {
    if (!module2DataTypes.containsKey(module))
      module2DataTypes.put(module, new ClassifierCollections.EListImpl<>());
    module2DataTypes.get(module).add(udt);
  }

  public EList<EClassifier> datatypes(String module) {
    if (!module2DataTypes.containsKey(module))
      return ClassifierCollections.EMPTY_LIST;
    return module2DataTypes.get(module);
  }

  private static HashMap<String, EList<EClassifier>> module2UserDefinedTypes = new HashMap<>();
  public void registerUserDefinedType(String module, EClassifier udt) {
    if (!module2UserDefinedTypes.containsKey(module))
      module2UserDefinedTypes.put(module, new ClassifierCollections.EListImpl<>());
    module2UserDefinedTypes.get(module).add(udt);
  }

  public EList<EClassifier> userDefinedTypes(String module) {
    if (!module2UserDefinedTypes.containsKey(module))
      return ClassifierCollections.EMPTY_LIST;
    return module2UserDefinedTypes.get(module);
  }

  public String dumpUdtPerModule() {
    StringBuilder b = new StringBuilder();
    module2UserDefinedTypes
        .entrySet()
        .stream()
        .forEach(e -> e.getValue()
            .stream()
            .forEach(p->b.append(e.getKey())
                .append(":")
                .append(p.getName())
                .append('\n')));
    return b.toString();
  }
}
