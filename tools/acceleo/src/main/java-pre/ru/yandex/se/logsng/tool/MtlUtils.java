package ru.yandex.se.logsng.tool;

import org.eclipse.emf.ecore.*;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * Created by minamoto on 28/06/16.
 */

public class MtlUtils {
  private static final Set<String> COMPRESSED_TYPES = new HashSet<>(Arrays.asList("KeyValue", "SectionedKeyValue", "JsonedString", "ZippedString"));

  public static boolean isObject(EStructuralFeature sf) {
    return EClass.class.isAssignableFrom(sf.getEType().getClass());
  }

  public static boolean isCompressedDataType(EStructuralFeature sf) {
    return (sf.getEType() instanceof EDataType && COMPRESSED_TYPES.contains(sf.getEType().getName()));
  }

  public static boolean isEnum(EStructuralFeature sf) {
    return EEnum.class.isAssignableFrom(sf.getEType().getClass());
  }

  public static String name(final ENamedElement element) {
    final EAnnotation annotation = extraData(element);
    return annotation.getDetails().stream().filter(e -> "name".equals(e.getKey())).findFirst().get().getValue();
  }

  private static EAnnotation extraData(ENamedElement element) {
    return element.getEAnnotation(Misc.HTTP_ORG_ECLIPSE_EMF_ECORE_UTIL_EXTENDED_META_DATA);
  }

  public static boolean vectorized(EStructuralFeature sf) {
    return sf.getLowerBound() > 1 || sf.getUpperBound() > 1 || sf.getUpperBound() == -1;
  }

  public static int indexOf(Collection list, Object sf) {
    int i = 0;
    for (Object feature : list) {
      if (feature.equals(sf))
        return i;
      i++;
    }

    return -1;
  }
}
