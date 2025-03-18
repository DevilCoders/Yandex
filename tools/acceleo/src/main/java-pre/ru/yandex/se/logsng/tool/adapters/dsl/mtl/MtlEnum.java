package ru.yandex.se.logsng.tool.adapters.dsl.mtl;

import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EEnumLiteral;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Enum;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.HashSet;
import java.util.Set;

/**
 * Created by astelmak on 12.07.16.
 */
public abstract class MtlEnum extends MtlType implements Enum {
    private final HashSet<String> literals = new HashSet<>();

    protected MtlEnum(TypeKey key, EEnum en) {
        super(key);

        for (EEnumLiteral l : en.getELiterals())
            this.literals.add(l.getLiteral());
    }

    @Override
    public Set<String> getEnumLiterals() {
        return literals;
    }
}
