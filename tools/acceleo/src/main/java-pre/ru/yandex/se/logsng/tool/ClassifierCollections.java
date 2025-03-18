package ru.yandex.se.logsng.tool;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EClassifier;

import java.util.ArrayList;

/**
 * Created by minamoto on 02/11/15.
 */
public class ClassifierCollections {
  private static final EList<EClassifier> EVENTS = new EListImpl<EClassifier>();

  public static void addEvent(EClass event) {
    EList<EClass> parents =  event.getEAllSuperTypes();
    if (!parents.get(parents.size() - 1).getName().equals("Event"))
      throw new RuntimeException("expected event successors");
    EVENTS.add(event);
  }

  public static EList<EClassifier> getEvents() {
    return EVENTS; /* heh? modifieble reference */
  }

  public final static EList<EClassifier> EMPTY_LIST = new EListImpl<>();
  public static class EListImpl<T> extends ArrayList<T> implements EList<T> {

    public void move(int i, T t) {
      throw new RuntimeException("unimplemented");
    }

    public T move(int i, int i1) {
      throw new RuntimeException("unimplemented");
    }
  }
}
