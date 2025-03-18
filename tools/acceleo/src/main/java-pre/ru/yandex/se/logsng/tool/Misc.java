package ru.yandex.se.logsng.tool;

import org.eclipse.emf.common.util.BasicEMap;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.impl.EAnnotationImpl;
import org.eclipse.emf.ecore.impl.EStringToStringMapEntryImpl;
import org.eclipse.emf.ecore.impl.EStructuralFeatureImpl;
import org.eclipse.emf.ecore.util.EObjectContainmentWithInverseEList;
import org.eclipse.ocl.ecore.impl.PrimitiveTypeImpl;

import java.util.*;

/**
 * Created by minamoto on 20/06/16.
 */
public class Misc {

  public static final String HTTP_ORG_ECLIPSE_EMF_ECORE_UTIL_EXTENDED_META_DATA = "http:///org/eclipse/emf/ecore/util/ExtendedMetaData";

  static class MagicStructuralFeature extends EStructuralFeatureImpl {
    @Override
    public EList<EAnnotation> getEAnnotations() {
      return super.getEAnnotations();
    }

    @Override
    public EAnnotation getEAnnotation(String source) {
      return super.getEAnnotation(source);
    }

    MagicStructuralFeature(final String name, final String type) {
      this.name = name;
      eType = new PrimitiveTypeImpl() {
        @Override
        public String getName() {
          return type;
        }
      };
      eAnnotations = new EObjectContainmentWithInverseEList<>(EAnnotation.class, this, -1, -1);
      eAnnotations.add(new EAnnotationImpl() {
        {
          source = HTTP_ORG_ECLIPSE_EMF_ECORE_UTIL_EXTENDED_META_DATA;
          details = new BasicEMap<String, String>();
          details.add(new EStringToStringMapEntryImpl(){
            {
              key = "name";
              value = name;
            }
          });
        }
      });
    }
  }

  /* SCARAB-329: here should be scarab:type, scarab:version */
  final static MagicStructuralFeature TYPE = new MagicStructuralFeature("type", "String");
  final static MagicStructuralFeature VERSION = new MagicStructuralFeature("version", "Integer");

  final static MagicStructuralFeature COMPRESSED = new MagicStructuralFeature("compressed", "Boolean");
  final static MagicStructuralFeature DATA = new MagicStructuralFeature("data", "String");

  final static Map<String, MagicStructuralFeature> NAME_2_STRUCTURAL_FEATURE = new HashMap<>();
  final static List<EStructuralFeature> COMPRESSED_TYPE = Arrays.asList(Misc.COMPRESSED, Misc.DATA);
  static {
    NAME_2_STRUCTURAL_FEATURE.put("type", TYPE);
    NAME_2_STRUCTURAL_FEATURE.put("version", VERSION);
    NAME_2_STRUCTURAL_FEATURE.put("compressed", COMPRESSED);
    NAME_2_STRUCTURAL_FEATURE.put("data", DATA);
  }
  public static LinkedHashSet<EStructuralFeature> magicFeatures() {
    LinkedHashSet<EStructuralFeature> list = new LinkedHashSet<>();
    list.add(TYPE);
    list.add(VERSION);
    return list;
  }

  public static String debugStructuralFeature(EStructuralFeature feature) {
    return "";
  }

  public static String debugMembers(EStructuralFeature feature, Object features) {
    return "";
  }
}
