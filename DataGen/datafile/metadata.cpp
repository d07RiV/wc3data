#include "metadata.h"
#include "id.h"

MetaData::MetaData(File file)
  : slk_(file)
{
  if (!slk_.valid()) return;

  cols_[ID] = slk_.columnIndex("ID");
  cols_[FIELD] = slk_.columnIndex("field");
  cols_[INDEX] = slk_.columnIndex("index");
  cols_[REPEAT] = slk_.columnIndex("repeat");
  cols_[APPENDINDEX] = slk_.columnIndex("appendIndex");
  cols_[DATA] = slk_.columnIndex("data");
  cols_[CATEGORY] = slk_.columnIndex("category");
  cols_[DISPLAY] = slk_.columnIndex("displayName");
  cols_[SORT] = slk_.columnIndex("sort");
  cols_[TYPE] = slk_.columnIndex("type");
  cols_[USEUNIT] = slk_.columnIndex("useUnit");
  cols_[USEHERO] = slk_.columnIndex("useHero");
  cols_[USEBUILDING] = slk_.columnIndex("useBuilding");
  cols_[USEITEM] = slk_.columnIndex("useItem");
  cols_[USESPECIFIC] = slk_.columnIndex("useSpecific");
  cols_[STRINGEXT] = slk_.columnIndex("stringExt");
  if (cols_[ID] < 0 || cols_[FIELD] < 0 || cols_[INDEX] < 0) {
    return;
  }

  for (size_t i = 0; i < slk_.rows(); ++i) {
    uint32 id = idFromString(slk_.item(i, cols_[ID]));
    rows_[id] = i;
    ids_[slk_.item(i, cols_[FIELD])] = id;
  }
}
