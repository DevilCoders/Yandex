package ru.yandex.se.logsng.tool;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;

import java.io.IOException;
import java.util.*;

import org.eclipse.emf.ecore.*;
import org.eclipse.emf.ecore.util.EcoreUtil;
import org.eclipse.emf.ecore.util.ExtendedMetaData;
import org.eclipse.xsd.*;
import org.eclipse.xsd.ecore.XSDEcoreBuilder;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;
import org.eclipse.emf.ecore.xmi.impl.XMIResourceFactoryImpl;
import org.eclipse.xsd.impl.XSDConcreteComponentImpl;
import org.eclipse.xsd.impl.XSDParticleImpl;
import org.w3c.dom.Element;


/**
 * Created by minamoto on 02/09/15.
 */
public class Xsd2EResource {
  public static void convert(final String[] sourcename, final String target) {
    XSDEcoreBuilder xsdEcoreBuilder = new Builder();
    ResourceSet resourceSet = new ResourceSetImpl();

    final Collection<URI> uris = new ArrayDeque<>();
    for (String source:sourcename) {
      System.out.println("convert: " + source);
      uris.add(URI.createFileURI(source));
    }

    Collection eCorePackages = xsdEcoreBuilder.generate(uris);
    resourceSet.getResourceFactoryRegistry().getExtensionToFactoryMap().put("ecore", new XMIResourceFactoryImpl());
    Resource resource = resourceSet.createResource(URI.createFileURI(target));

    for (Iterator iter = eCorePackages.iterator(); iter.hasNext();) {
      final Object next = iter.next();
      if (next instanceof EPackage) {
        EPackage element = (EPackage) next;
        resource.getContents().add(element);
      }
      else if (next instanceof ArrayList){
        for (ArrayList<String> msg:(ArrayList<ArrayList<String>>) next){
          if (msg.get(1).startsWith("Error:"))
            throw new RuntimeException(msg.get(1));
          System.err.println("WARN: " + msg.get(1));
        }
      }
    }

    try {
      resource.save(null);
    } catch (IOException e) {
      throw new RuntimeException(e);
    }

    System.out.println("Finished");
  }

  public static void main(String[] arg){
    convert(new String[]{"xsd/common.xsd", "tests/semi_double/a.xsd", "tests/semi_double/b.xsd"}, "/Users/minamoto/ws/scarab/target/generated-sources/LoggingData.ecore");
  }

  public static class Builder extends XSDEcoreBuilder {

    private String getXSDName(EModelElement element) {
      return element.getEAnnotation("http:///org/eclipse/emf/ecore/util/ExtendedMetaData").getDetails().get("name");
    }
    @Override
    protected void initialize(final EStructuralFeature eStructuralFeature, XSDFeature xsdFeature, XSDComponent xsdComponent) {
      super.initialize(eStructuralFeature, xsdFeature, xsdComponent);
      final String value = xsdComponent.getElement().getAttribute("nillable");

      if (value != null
          && Boolean.parseBoolean(value)
          && (xsdComponent instanceof XSDParticle)
          && !((XSDParticle) xsdComponent).isSetMinOccurs()
          && !((XSDParticle) xsdComponent).isSetMaxOccurs()) {
        EcoreUtil.setAnnotation(eStructuralFeature, "http:///org/eclipse/emf/ecore/util/ExtendedMetaData", "nillable", "true");
      }
      checkAndSetMetaDataAnnotation(eStructuralFeature, xsdComponent, "default");
      checkAndSetMetaDataAnnotation(eStructuralFeature, xsdComponent, "fixed");

      if (xsdComponent.getContainer() instanceof XSDModelGroup && xsdComponent.getContainer().getContainer() instanceof XSDModelGroupDefinition) {
        final XSDModelGroupDefinition def = (XSDModelGroupDefinition) xsdComponent.getContainer().getContainer();
        String groupName = def.getName();
        final String xsdName = getXSDName(eStructuralFeature);
        EClass clazz = eStructuralFeature.getEContainingClass();
        final String xsdClassName = getXSDName(clazz);
        XSDConcreteComponent xsdClassDefinition = xsdComponent.getRootContainer().resolveComplexTypeDefinition(xsdClassName);
        EList<XSDConcreteComponent> components = ((XSDConcreteComponentImpl)xsdClassDefinition).getXSDContents();
        for (XSDConcreteComponent component:components) {
          if (component instanceof XSDParticleImpl) {
            final EList<EObject> objects = ((XSDParticleImpl) component).getXSDContents().get(0).eContents();
            for(EObject obj:objects) {
              XSDParticle particle = ((XSDParticle)obj);
              for (EObject obj0:particle.eContents()){
                if (!(obj0 instanceof XSDModelGroupDefinition))
                  continue;
                XSDModelGroupDefinition reference = (XSDModelGroupDefinition)obj0;
                if (!reference.getElement().getAttribute("ref").equals(groupName) || reference.getAnnotation() == null)
                  continue;
                EList<Element> elements = reference.getAnnotation().getApplicationInformation();
                for(Element element:elements) {
                  String source = element.getAttribute("source");
                  if (source.startsWith("scarab:")) {
                    EcoreUtil.setAnnotation(eStructuralFeature, source, "appinfo", element.getTextContent().trim());
                  }
                }
              }
            }
          }
        }
      }
    }

    @Override
    protected void resolveNameConflicts() {
      for (EPackage ePackage : targetNamespaceToEPackageMap.values())
      {
        HashSet<String> commonClassifier = new HashSet<>();
        ePackage.getEClassifiers().stream()
            .filter(p -> "common".equals(moduleName(p)))
            .forEach(p -> {
              final String qualifiedName = classifier2ModuleQualifiedName("common", p.getName());
              checkAndAddName(commonClassifier, qualifiedName, p.getName() + " already declared in module: common");
            });

        HashSet<String> moduleClassifier = new HashSet<>();
        ePackage.getEClassifiers().stream()
            .filter(p ->moduleName(p) != null)
            .filter(p -> !"common".equals(moduleName(p)))
            .forEach(p -> {
              String module = moduleName(p);
              String name = p.getName();
              String qualifiedName = classifier2ModuleQualifiedName(module, name);
              checkAndAddName(moduleClassifier, qualifiedName, p.getName() + " already declared in module:" + module);
              isAlreadyDefined(commonClassifier, classifier2ModuleQualifiedName("common", name), qualifiedName + " already declared in module: common");
            });
      }
    }

    private void checkAndAddName(HashSet<String> set, String qualifiedName, String message) {
      isAlreadyDefined(set, qualifiedName, message);
      set.add(qualifiedName);

    }

    private void isAlreadyDefined(HashSet<String> set, String qualifiedName, String message) {
      if (set.contains(qualifiedName))
        throw new RuntimeException(message);
    }

    private String moduleName(EClassifier p) {
      if (p == null || p.getEAnnotation("scarab:builtin:module") == null)
        return null;
      return p.getEAnnotation("scarab:builtin:module").getDetails().get("appinfo");
    }

    private String classifier2ModuleQualifiedName(String module, String name) {
      if (module == null || name == null)
        return null;
      return module + "_" + name;
    }

    public EClassifier getEClassifier(XSDTypeDefinition xsdTypeDefinition) {
      EClassifier classifier = super.getEClassifier(xsdTypeDefinition);
      final XSDConcreteComponent container = xsdTypeDefinition.getContainer();
      if ((container instanceof XSDSchema)
          && classifier.getEAnnotation("scarab:builtin:module") == null) {
        final XSDSchema schema = (XSDSchema) container;
        String location = schema.getSchemaLocation();
        int slashIndex = location.lastIndexOf('/');
        int extentionIndex = location.lastIndexOf('.');
        EcoreUtil.setAnnotation(classifier, "scarab:builtin:module", "appinfo", location.substring(slashIndex + 1, extentionIndex));
      }
      return classifier;
    }
    private void checkAndSetMetaDataAnnotation(EStructuralFeature eStructuralFeature, XSDComponent xsdComponent, String name) {
      final String attributeValue = xsdComponent.getElement().getAttribute(name);
      if (attributeValue != null && attributeValue.length() != 0) {
        EcoreUtil.setAnnotation(eStructuralFeature, "http:///org/eclipse/emf/ecore/util/ExtendedMetaData", name, attributeValue);
      }
    }

    /**
     * this method were overrided to avoid javiness of nullable support.
     */

    @Override
    protected EStructuralFeature createFeature
        (EClass eClass, String name, EClassifier type, XSDComponent xsdComponent, int minOccurs, int maxOccurs)
    {
      if (xsdComponent != null)
      {
        XSDSchema containingXSDSchema = xsdComponent.getSchema();
        if (containingXSDSchema != null && !xsdSchemas.contains(containingXSDSchema))
        {
          xsdSchemas.add(containingXSDSchema);
          addInput(containingXSDSchema);
          validate(containingXSDSchema);
        }
      }
      else if (extendedMetaData.getContentKind(eClass) == ExtendedMetaData.MIXED_CONTENT)
      {
        if (type == EcorePackage.Literals.EFEATURE_MAP_ENTRY)
        {
          EAttribute eAttribute = EcoreFactory.eINSTANCE.createEAttribute();
          setAnnotations(eAttribute, xsdComponent);
          eAttribute.setName(Character.toLowerCase(name.charAt(0)) + name.substring(1));
          if (maxOccurs != 1)
          {
            eAttribute.setUnique(false);
          }
          eAttribute.setEType(type);
          eAttribute.setLowerBound(minOccurs);
          eAttribute.setUpperBound(maxOccurs);
          eClass.getEStructuralFeatures().add(eAttribute);
          extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.ELEMENT_WILDCARD_FEATURE);
          extendedMetaData.setName(eAttribute, ":" + eAttribute.getName());
          return eAttribute;
        }
        else
        {
          EReference eReference = EcoreFactory.eINSTANCE.createEReference();
          setAnnotations(eReference, xsdComponent);
          eReference.setName(name);
          eReference.setEType(EcorePackage.Literals.ESTRING_TO_STRING_MAP_ENTRY);
          eReference.setLowerBound(0);
          eReference.setUpperBound(-1);
          eReference.setContainment(true);
          eReference.setResolveProxies(false);
          eReference.setTransient(true);
          eClass.getEStructuralFeatures().add(eReference);
          extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.ATTRIBUTE_FEATURE);
          return eReference;
        }
      }

      if (type instanceof EClass)
      {
        EReference eReference = EcoreFactory.eINSTANCE.createEReference();
        setAnnotations(eReference, xsdComponent);
        eReference.setName(Character.toLowerCase(name.charAt(0)) + name.substring(1));
        eReference.setEType(type);
        eReference.setLowerBound(minOccurs);
        eReference.setUpperBound(maxOccurs);

        eClass.getEStructuralFeatures().add(eReference);
        if (xsdComponent == null || xsdComponent instanceof XSDSimpleTypeDefinition)
        {
          extendedMetaData.setName(eReference, ":" + eClass.getEAllStructuralFeatures().indexOf(eReference));
          extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.SIMPLE_FEATURE);
          eReference.setResolveProxies(!isLocalReferenceType((XSDSimpleTypeDefinition)xsdComponent));
        }
        else
        {
          map(xsdComponent, eReference);
          if (xsdComponent instanceof XSDParticle)
          {
            eReference.setContainment(true);
            eReference.setResolveProxies(false);

            XSDParticle xsdParticle = (XSDParticle)xsdComponent;

            XSDTerm xsdTerm = ((XSDParticle)xsdComponent).getTerm();
            if (xsdTerm instanceof XSDElementDeclaration)
            {
              XSDElementDeclaration xsdElementDeclaration = (XSDElementDeclaration)xsdTerm;
              extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.ELEMENT_FEATURE);
              extendedMetaData.setName(eReference, xsdElementDeclaration.getName());
              extendedMetaData.setNamespace(eReference, xsdElementDeclaration.getTargetNamespace());

              XSDTypeDefinition xsdType = getEffectiveTypeDefinition(xsdParticle, xsdElementDeclaration);
              if (xsdType instanceof XSDSimpleTypeDefinition)
              {
                eReference.setContainment(false);
                eReference.setResolveProxies(!isLocalReferenceType((XSDSimpleTypeDefinition)xsdType));
              }
/*
              if (maxOccurs == 1 && xsdElementDeclaration.isNillable())
              {
                eReference.setUnsettable(true);
              }
*/
              if (xsdElementDeclaration.isAbstract())
              {
                eReference.setChangeable(false);
              }

              String opposite = getEcoreAttribute(xsdParticle, "opposite");
              if (opposite != null)
              {
                eReferenceToOppositeNameMap.put(eReference, opposite);
              }

              String key = getEcoreAttribute(xsdParticle, "keys");
              if (key != null)
              {
                List<String> keyNames = new ArrayList<String>();
                for (StringTokenizer stringTokenizer = new StringTokenizer(key); stringTokenizer.hasMoreTokens(); )
                {
                  keyNames.add(stringTokenizer.nextToken());
                }
                eReferenceToKeyNamesMap.put(eReference, keyNames);
              }
            }
            else if (xsdTerm instanceof XSDWildcard)
            {
              // EATM shouldn't happen
              XSDWildcard xsdWildcard = (XSDWildcard)xsdTerm;
              extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.ELEMENT_WILDCARD_FEATURE);
              extendedMetaData.setWildcards(eReference, getWildcards(xsdWildcard));
              extendedMetaData.setProcessingKind(eReference, xsdWildcard.getProcessContents().getValue() + 1);
              extendedMetaData.setName(eReference, ":" + eClass.getEAllStructuralFeatures().indexOf(eReference));
            }
            else
            {
              extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.GROUP_FEATURE);
              extendedMetaData.setName(eReference, ":" + eClass.getEAllStructuralFeatures().indexOf(eReference));
            }
          }
          else if (xsdComponent instanceof XSDElementDeclaration)
          {
            XSDElementDeclaration xsdElementDeclaration = (XSDElementDeclaration)xsdComponent;
            eReference.setContainment(true);
            eReference.setResolveProxies(false);
            extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.ELEMENT_FEATURE);
            extendedMetaData.setName(eReference, xsdElementDeclaration.getName());
            extendedMetaData.setNamespace(eReference, xsdElementDeclaration.getTargetNamespace());

            XSDElementDeclaration substitutionGroupAffiliation = xsdElementDeclaration.getSubstitutionGroupAffiliation();
            if (substitutionGroupAffiliation != null)
            {
              EStructuralFeature affiliation = getEStructuralFeature(substitutionGroupAffiliation);
              extendedMetaData.setAffiliation(eReference, affiliation);
            }
            XSDTypeDefinition xsdType = getEffectiveTypeDefinition(null, xsdElementDeclaration);
            if (xsdType instanceof XSDSimpleTypeDefinition)
            {
              eReference.setResolveProxies(!isLocalReferenceType((XSDSimpleTypeDefinition)xsdType));
            }
/*
            if (maxOccurs == 1 && xsdElementDeclaration.isNillable())
            {
              eReference.setUnsettable(true);
            }
*/
            if (xsdElementDeclaration.isAbstract())
            {
              eReference.setChangeable(false);
            }
          }
          else if (xsdComponent instanceof XSDAttributeUse)
          {
            String opposite = getEcoreAttribute(xsdComponent, "opposite");
            if (opposite != null)
            {
              eReferenceToOppositeNameMap.put(eReference, opposite);
            }

            String key = getEcoreAttribute(xsdComponent, "keys");
            if (key != null)
            {
              List<String> keyNames = new ArrayList<String>();
              for (StringTokenizer stringTokenizer = new StringTokenizer(key); stringTokenizer.hasMoreTokens(); )
              {
                keyNames.add(stringTokenizer.nextToken());
              }
              eReferenceToKeyNamesMap.put(eReference, keyNames);
            }

            XSDAttributeUse xsdAttributeUse = (XSDAttributeUse)xsdComponent;
            XSDAttributeDeclaration xsdAttributeDeclaration = xsdAttributeUse.getAttributeDeclaration();
            extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.ATTRIBUTE_FEATURE);
            extendedMetaData.setName(eReference, xsdAttributeDeclaration.getName());
            extendedMetaData.setNamespace(eReference, xsdAttributeDeclaration.getTargetNamespace());
            eReference.setResolveProxies
                (!isLocalReferenceType((XSDSimpleTypeDefinition)getEffectiveTypeDefinition(xsdAttributeUse, xsdAttributeDeclaration)));
          }
          else if (xsdComponent instanceof XSDAttributeDeclaration)
          {
            XSDAttributeDeclaration xsdAttributeDeclaration = (XSDAttributeDeclaration)xsdComponent;
            extendedMetaData.setFeatureKind(eReference, ExtendedMetaData.ATTRIBUTE_FEATURE);
            extendedMetaData.setName(eReference, xsdAttributeDeclaration.getName());
            extendedMetaData.setNamespace(eReference, xsdAttributeDeclaration.getTargetNamespace());
            eReference.setResolveProxies
                (!isLocalReferenceType((XSDSimpleTypeDefinition)getEffectiveTypeDefinition(null, xsdAttributeDeclaration)));
          }
        }

        return eReference;
      }
      else
      {
        EAttribute eAttribute = EcoreFactory.eINSTANCE.createEAttribute();
        setAnnotations(eAttribute, xsdComponent);
        eAttribute.setName(Character.toLowerCase(name.charAt(0)) + name.substring(1));
        if (maxOccurs != 1)
        {
          eAttribute.setUnique(false);
        }
        eAttribute.setEType(type);
        eAttribute.setLowerBound(minOccurs);
        eAttribute.setUpperBound(maxOccurs);
        eClass.getEStructuralFeatures().add(eAttribute);

        if (xsdComponent == null || xsdComponent instanceof XSDSimpleTypeDefinition)
        {
          extendedMetaData.setName(eAttribute, ":" + eClass.getEAllStructuralFeatures().indexOf(eAttribute));
          extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.SIMPLE_FEATURE);
        }
        else
        {
          map(xsdComponent, eAttribute);
          if (xsdComponent instanceof XSDAttributeUse)
          {
            XSDAttributeUse xsdAttributeUse = (XSDAttributeUse)xsdComponent;
            XSDAttributeDeclaration xsdAttributeDeclaration = xsdAttributeUse.getAttributeDeclaration();
            extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.ATTRIBUTE_FEATURE);
            extendedMetaData.setName(eAttribute, xsdAttributeDeclaration.getName());
            extendedMetaData.setNamespace(eAttribute, xsdAttributeDeclaration.getTargetNamespace());

            String defaultValue = getEcoreAttribute(xsdComponent, "default");
            if (defaultValue == null)
            {
              defaultValue = xsdAttributeUse.getLexicalValue();
            }
            setDefaultValueLiteral(eAttribute, defaultValue);
            initialize(eAttribute, (XSDSimpleTypeDefinition)getEffectiveTypeDefinition(xsdAttributeUse, xsdAttributeDeclaration));
          }
          else if (xsdComponent instanceof XSDAttributeDeclaration)
          {
            XSDAttributeDeclaration xsdAttributeDeclaration = (XSDAttributeDeclaration)xsdComponent;
            extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.ATTRIBUTE_FEATURE);
            extendedMetaData.setName(eAttribute, xsdAttributeDeclaration.getName());
            extendedMetaData.setNamespace(eAttribute, xsdAttributeDeclaration.getTargetNamespace());

            setDefaultValueLiteral(eAttribute, xsdAttributeDeclaration.getLexicalValue());
            initialize(eAttribute, (XSDSimpleTypeDefinition)getEffectiveTypeDefinition(null, xsdAttributeDeclaration));
          }
          else if (xsdComponent instanceof XSDParticle)
          {
            XSDTerm xsdTerm = ((XSDParticle)xsdComponent).getTerm();
            if (xsdTerm instanceof XSDElementDeclaration)
            {
              XSDElementDeclaration xsdElementDeclaration = (XSDElementDeclaration)xsdTerm;
              extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.ELEMENT_FEATURE);
              extendedMetaData.setName(eAttribute, xsdElementDeclaration.getName());
              extendedMetaData.setNamespace(eAttribute, xsdElementDeclaration.getTargetNamespace());

              setDefaultValueLiteral(eAttribute, xsdElementDeclaration.getLexicalValue());
              XSDTypeDefinition xsdType = getEffectiveTypeDefinition(xsdComponent, xsdElementDeclaration);
              if (xsdType instanceof XSDSimpleTypeDefinition)
              {
                initialize(eAttribute, (XSDSimpleTypeDefinition)xsdType);
              }
              // minamoto@ this is too Java specific
/*
              if (xsdElementDeclaration.isNillable())
              {
                if (!canSupportNull((EDataType)type))
                {
                  eAttribute.setEType(type = typeToTypeObjectMap.get(type));
                }
                if (maxOccurs == 1)
                {
                  eAttribute.setUnsettable(true);
                }
              }
*/
              if (xsdElementDeclaration.isAbstract())
              {
                eAttribute.setChangeable(false);
              }
            }
            else if (xsdTerm instanceof XSDWildcard)
            {
              XSDWildcard xsdWildcard = (XSDWildcard)xsdTerm;
              extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.ELEMENT_WILDCARD_FEATURE);
              extendedMetaData.setWildcards(eAttribute, getWildcards(xsdWildcard));
              extendedMetaData.setProcessingKind(eAttribute, xsdWildcard.getProcessContents().getValue() + 1);
              extendedMetaData.setName(eAttribute, ":" + eClass.getEAllStructuralFeatures().indexOf(eAttribute));
            }
            else
            {
              extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.GROUP_FEATURE);
            }
          }
          else if (xsdComponent instanceof XSDWildcard)
          {
            XSDWildcard xsdWildcard = (XSDWildcard)xsdComponent;
            extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.ATTRIBUTE_WILDCARD_FEATURE);
            extendedMetaData.setWildcards(eAttribute, getWildcards(xsdWildcard));
            extendedMetaData.setProcessingKind(eAttribute, xsdWildcard.getProcessContents().getValue() + 1);
            extendedMetaData.setName(eAttribute, ":" + eClass.getEAllStructuralFeatures().indexOf(eAttribute));
          }
          else if (xsdComponent instanceof XSDElementDeclaration)
          {
            XSDElementDeclaration xsdElementDeclaration = (XSDElementDeclaration)xsdComponent;
            extendedMetaData.setFeatureKind(eAttribute, ExtendedMetaData.ELEMENT_FEATURE);
            extendedMetaData.setName(eAttribute, xsdElementDeclaration.getName());
            extendedMetaData.setNamespace(eAttribute, xsdElementDeclaration.getTargetNamespace());

            setDefaultValueLiteral(eAttribute, xsdElementDeclaration.getLexicalValue());
            XSDTypeDefinition xsdType = getEffectiveTypeDefinition(null, xsdElementDeclaration);
            if (xsdType instanceof XSDSimpleTypeDefinition)
            {
              initialize(eAttribute, (XSDSimpleTypeDefinition)xsdType);
            }

            XSDElementDeclaration substitutionGroupAffiliation = xsdElementDeclaration.getSubstitutionGroupAffiliation();
            if (substitutionGroupAffiliation != null)
            {
              EStructuralFeature affiliation = getEStructuralFeature(substitutionGroupAffiliation);
              extendedMetaData.setAffiliation(eAttribute, affiliation);
            }
/*
            if (xsdElementDeclaration.isNillable() && !canSupportNull((EDataType)type))
            {
              eAttribute.setEType(type = typeToTypeObjectMap.get(type));
              if (maxOccurs == 1)
              {
                eAttribute.setUnsettable(true);
              }
            }
*/
            if (xsdElementDeclaration.isAbstract())
            {
              eAttribute.setChangeable(false);
            }
          }
        }

        if (maxOccurs == 1 && (type.getDefaultValue() != null || eAttribute.getDefaultValueLiteral() != null))
        {
          eAttribute.setUnsettable(true);
        }

        return eAttribute;
      }
    }

  }
}
