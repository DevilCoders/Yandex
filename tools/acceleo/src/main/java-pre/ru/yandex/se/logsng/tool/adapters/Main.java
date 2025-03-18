package ru.yandex.se.logsng.tool.adapters;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EClassifier;
import ru.yandex.se.logsng.tool.ClassifierCollections;
import ru.yandex.se.logsng.tool.adapters.dsl.DSL;
import ru.yandex.se.logsng.tool.adapters.dsl.DSLCompiler;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlType;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.ObjStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Event;
import ru.yandex.se.logsng.tool.adapters.dsl.types.InterfaceAdapter;

import java.util.Collection;
import java.util.HashSet;

public abstract class Main {
    protected abstract InterfaceAdapter createAdapter(EClass adapter);

    private static boolean equalsNullableString(String s1, String s2) {
        return s1 == s2 || (s1 != null && s1.equals(s2));
    }

    private static boolean ensureEventNeeded(InterfaceAdapter adapter, Event event) {
        return "common".equals(event.getKey().getModule()) ||
                equalsNullableString(adapter.getKey().getModule(), event.getKey().getModule());
    }

    protected abstract Event getEvent(EClass clazz);

    private Collection<Event> scanEvents(InterfaceAdapter adapter) {
        HashSet<Event> result = new HashSet<>();

        EList<EClassifier> events = ClassifierCollections.getEvents();

        for (EClassifier cl : events) {
            EClass eventClass = (EClass)cl;

            Event event = getEvent(eventClass);

            if (ensureEventNeeded(adapter, event))
                result.add(event);
        }

        return result;
    }

    protected abstract ObjStatement createBaseObject();

    public String generate(EClass adapterClass, String compilerClassName) throws Exception {
        EAnnotation ann = adapterClass.getEAnnotation("scarab:query");

        if (ann == null)
            throw new IllegalStateException("'scarab:query' annotation in '" + MtlType.getXSDTypeName(adapterClass) +
                                            "' adapter is undefined");

        String query = ann.getDetails().get("appinfo");

        if (query == null || (query = query.trim()).isEmpty())
            throw new IllegalStateException("'appinfo' in '" + MtlType.getXSDTypeName(adapterClass) +
                    "' adapter is undefined");

        InterfaceAdapter adapter = createAdapter(adapterClass);
        Collection<Event> events = scanEvents(adapter);
        ObjStatement event = createBaseObject();

        DSL dsl = DSLCompiler.compile(query, adapterClass.getName(), Class.forName(compilerClassName));

        dsl.setParameters(adapter, events);

        return dsl.eval(event).toString(new StringBuilder(16 * 1024 * 1024)).toString();
    }
}
