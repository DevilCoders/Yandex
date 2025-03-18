package ru.yandex.se.logsng.tool;

import org.apache.commons.lang3.tuple.Pair;
import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EStructuralFeature;

import java.util.*;
import java.util.stream.Collectors;

/**
 * Created by minamoto on 15/06/16.
 */
public class Symbols {
  private static HashMap<String, Integer> STR_2_STRING_INDEX_TABLE = new HashMap<>();
  private static HashSet<String> SYMBOLS = new HashSet<>();
  private static List<String> SYMBOLS_LIST = new ArrayList<>();

  static {
    /* please enhance me in SCARAB-580 */
    addStringTableEntry("type"); /* base symbol table */
    addStringTableEntry("version"); /* base symbol table */
    addStringTableEntry("timestamp"); /* base symbol table */
    addStringTableEntry("provider"); /* base symbol table */
    addStringTableEntry("compressed");
    addStringTableEntry("data");
  }

  public static void addStringTableEntry(String symbolName) {
    if (SYMBOLS.add(symbolName)) {
      SYMBOLS_LIST.add(symbolName);
      STR_2_STRING_INDEX_TABLE.put(symbolName, SYMBOLS_LIST.indexOf(symbolName));
    }
  }

  public static int stringTableSize() {
    return SYMBOLS_LIST.size();
  }

  public static String stringTableEntry(Integer index) {
    return SYMBOLS_LIST.get(index);
  }

  public static EList<String> stringTableEntries() {
    return new BasicEList<String>(SYMBOLS_LIST);
    //return SYMBOLS_LIST.get(index);
  }

  public static int stringTableEntryIndex(String symbolName) {
    return STR_2_STRING_INDEX_TABLE.get(symbolName);
  }

  static class Symbol {
    final int stringTableEntry;
    final int nextSymbolEntry;
    final int childSymbolEntry;

    final boolean arraySymbolEntry;

    Symbol(int stringTableEntry, int nextSymbolEntry, int childSymbolEntry, boolean arraySymbolEntry) {
      this.stringTableEntry = stringTableEntry;
      this.nextSymbolEntry = nextSymbolEntry;
      this.childSymbolEntry = childSymbolEntry;
      this.arraySymbolEntry = arraySymbolEntry;
    }
    Symbol(int stringTableEntry, int nextSymbolEntry, int childSymbolEntry) {
      this(stringTableEntry, nextSymbolEntry, childSymbolEntry, false);
    }
  }

  static class EventKey extends Pair<String, Integer> {
    final String evenName;
    final Integer version;

    EventKey(String evenName, Integer version) {
      this.evenName = evenName;
      this.version = version;
    }

    @Override
    public Integer setValue(Integer value) {
      throw new IllegalAccessError("unsupported method");
    }

    @Override
    public String getLeft() {
      return evenName;
    }

    @Override
    public Integer getRight() {
      return version;
    }
  }

  private final static HashMap<EventKey, List<Symbol>> SYMBOL_TABLES = new HashMap<>();

  static int index = 0;
  static EventKey eventKey;
  public static void startSymbolTable(String eventName, Integer version) {
    index = 0;
    eventKey = new EventKey(eventName, version);
    SYMBOL_TABLES.put(eventKey, new ArrayList<>());
  }

  public static void endSymbolTable(String eventName, Integer version) {
    eventKey = null;
  }

  public static void entrySymbolTable(String eventName, Integer version, Integer symbolIndex,
                                           Integer nextIndex, Integer childIndex, Boolean array) {
    /* System.err.println(eventName + " " + version
        + ":" + index++
        + " (" + symbolIndex + " " + SYMBOLS_LIST.get(symbolIndex) + ") "
        + nextIndex
        + " " + childIndex
        + " " + array);*/
    assert eventKey != null;
    assert eventKey.getKey() == eventName && eventKey.getValue() == version;
    assert SYMBOL_TABLES.get(eventKey).size() == index;
    SYMBOL_TABLES.get(eventKey).add(index++, new Symbol(symbolIndex, nextIndex, childIndex, array));
    assert (nextIndex == -1 && nextIndex == childIndex) || index == nextIndex || index == childIndex;
  }

  public void dump() {
    for (int i = 0, n = SYMBOLS_LIST.size(); i != n; ++i) {
      System.err.println(i + ":" + SYMBOLS_LIST.get(i));
    }
  }

  public static int symbolTableSize(final String eventName, final Integer version) {
    return SYMBOL_TABLES.get(new EventKey(eventName, version)).size();
  }

  public static Integer stringTableIndex4SymbolIndex(final String eventName, final Integer version, Integer symbolIndex) {
    return SYMBOL_TABLES.get(new EventKey(eventName, version)).get(symbolIndex).stringTableEntry;
  }

  public static Integer nextIndex4SymbolIndex(final String eventName, final Integer version, Integer symbolIndex) {
    return SYMBOL_TABLES.get(new EventKey(eventName, version)).get(symbolIndex).nextSymbolEntry;
  }

  public static Integer childIndex4SymbolIndex(final String eventName, final Integer version, Integer symbolIndex) {
    return SYMBOL_TABLES.get(new EventKey(eventName, version)).get(symbolIndex).childSymbolEntry;
  }

  public static Boolean arrayeness4SymbolIndex(final String eventName, final Integer version, Integer symbolIndex) {
    return SYMBOL_TABLES.get(new EventKey(eventName, version)).get(symbolIndex).arraySymbolEntry;
  }

  public static Integer lookupPathIndexInternal(final EClass clazz, Integer version, Object obj) {
    String name = MtlUtils.name(clazz);
    List<Symbol> symbols = SYMBOL_TABLES.get(new EventKey(name, version));
    List<EStructuralFeature> path = (List<EStructuralFeature>)obj;
    Symbol symbol = symbols.get(0);
    for (EStructuralFeature sf: path) {
      final String sfName = MtlUtils.name(sf);
      int symbolIndex = SYMBOLS_LIST.indexOf(sfName);
      if (symbolIndex == -1)
        throw new NoSuchElementException("ivalid path: " + sfName);

      while (symbol.stringTableEntry != symbolIndex) {
        symbol = symbols.get(symbol.nextSymbolEntry);
      }
      if (path.indexOf(sf) == path.size() - 1) {
        return symbols.indexOf(symbol);
      }
      symbol = symbols.get(symbol.childSymbolEntry);
    }
    throw new NoSuchElementException("can't find path: " + path2String(path));
  }

  private static String path2String(List<EStructuralFeature> path) {
    return path.stream().map(MtlUtils::name).collect(Collectors.joining("::"));
  }

  public static String snakeCase2CamelCase(final String str) {
    StringBuilder sb = new StringBuilder();
    boolean lastCharIsUnderscore = true; // first symbol must be in upper case
    for (char ch : str.toCharArray()) {
      if (ch == '_') {
        lastCharIsUnderscore = true;
      } else {
        if (lastCharIsUnderscore) {
          sb.append(Character.toUpperCase(ch));
        } else {
          sb.append(ch);
        }
        lastCharIsUnderscore = false;
      }
    }
    return sb.toString();
  }

  public static String buildSymbolTable(final String eventName, final Integer version, Object obj) {
    int index = 0;
    if (LinkedHashSet.class.isAssignableFrom(obj.getClass())) {
      LinkedHashSet<EStructuralFeature> members = (LinkedHashSet<EStructuralFeature>)obj;
      buildSymbolTable(eventName, version, members, index);
    }
    else {
      throw new RuntimeException("Unsupported type: call for minamoto@");
    }
    return "";
  }

  public static EStructuralFeature magic(String name) {
    return Misc.NAME_2_STRUCTURAL_FEATURE.get(name);
  }

  private static int buildSymbolTable(final String eventName, int version, final LinkedHashSet<EStructuralFeature> members, int index) {
    if (members.isEmpty())
      return index;

    EStructuralFeature[] features = members.toArray(new EStructuralFeature[0]);
    return buildSymbolTable2(eventName, version, Arrays.asList(features), index);
  }

  private static int buildSymbolTable2(String eventName, int version, List<EStructuralFeature> features, int index) {
    for (int i = 0, n = features.size(); i != n; ++i) {
      EStructuralFeature sf = features.get(i);
      int symbolIndex = stringTableEntryIndex(MtlUtils.name(sf));
      int nextIndex = i == (n - 1) ? -1 : calculateNext(sf) + index;
      int childIndex = calculateChild(sf, index);
      boolean vectorized = MtlUtils.vectorized(sf);
      entrySymbolTable(eventName, version, symbolIndex, nextIndex, childIndex, vectorized);
      if (MtlUtils.isObject(sf) || MtlUtils.isCompressedDataType(sf)) {

        buildSymbolTable2(eventName, version, MtlUtils.isObject(sf) ? toClass(sf).getEAllStructuralFeatures() : Misc.COMPRESSED_TYPE, index + 1);
      }
      index += (nextIndex - index);
    }
    return index;
  }

  private static int calculateChild(EStructuralFeature sf, int index) {
    return MtlUtils.isObject(sf) || MtlUtils.isCompressedDataType(sf)? 1 + index: -1;
  }

  private static int calculateNext(EStructuralFeature sf) {
    if (!MtlUtils.isObject(sf) && !MtlUtils.isCompressedDataType(sf))
      return 1;
    if (MtlUtils.isObject(sf))
      return depth(toClass(sf).getEStructuralFeatures());
    else
      return 2 + 1; /* compressedDataType: compressed + data */
  }

  private static EClass toClass(EStructuralFeature sf) {
    return (EClass) sf.getEType();
  }

  private static int depth(EList<EStructuralFeature> eStructuralFeatures) {
    if (eStructuralFeatures.isEmpty())
      return 1;
    return eStructuralFeatures.stream().map(sf -> {
      if (MtlUtils.isObject(sf))
        return depth(toClass(sf).getEStructuralFeatures());
      return 1;
    }).reduce(1, (a, b) -> {
      return a + b;
    });
  }

}
