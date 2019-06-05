#include <gtest/gtest.h>
#include <string>

#include "../include/vsqlite_serialize.h"
#include "../src/utils.h"

class OsqueryJsonTest : public ::testing::Test {
protected:
  virtual void SetUp() {  }
};

static const SPFieldDef fname = FieldDef::alloc(TSTRING, "name");
static const SPFieldDef fage = FieldDef::alloc(TINT32, "age");
static const SPFieldDef factive = FieldDef::alloc(TUINT8, "active");

// in order to be able to consistently traverse rows in same column order...

static std::vector<SPFieldDef> cols = { fname, fage, factive };

// this is very simple, no escaping.  Just a way to get a printable value
static std::string SimpleRowToJSONString(DynMap &row) {
  std::string s;
  int i=-1;
  for (auto colId : cols) {
    i++;

    DynVal &val = row[colId];
    if (!val.valid()) {
      continue;
    }

    if (!s.empty()) { s += ", "; }

    s += colId->name + ":";
    if (val.type() == TSTRING) {
      s += "\"" + val.as_s() + "\"";
    } else {
      s += val.as_s();
    }
  }
  return "{" + s + "}";
}


struct MyDiffResultsListener: public vsqlite::DiffResultsListener {
  virtual ~MyDiffResultsListener() {}

  void onAdded(DynMap &row) override {
    adds.push_back(SimpleRowToJSONString(row));
  }

  void onRemoved(DynMap &row) override {
    removes.push_back(SimpleRowToJSONString(row));
  }
  std::vector<std::string> adds;
  std::vector<std::string> removes;
};

static std::string gExpected1 = "[{\"active\":\"1\",\"age\":\"32\",\"name\":\"bob\"},{\"active\":\"0\",\"name\":\"Judy\"},{\"age\":\"3\",\"name\":\"Coco\"}]";

static std::string gExpected1_row1only = "[{\"active\":\"0\",\"name\":\"Judy\"}]";

static const std::vector<DynMap> &ExampleData1() {
  static std::vector<DynMap> _rows;
  if (_rows.empty()) {
    _rows.resize(3);
    DynMap &row = _rows[0];
    row[fname] = "bob";
    row[fage] = 32;
    row[factive] = true;

    DynMap &row2 = _rows[1];
    row2[fname] = "Judy";
    row2[fage] = DynVal(); // null / notset
    row2[factive] = false;

    DynMap &row3 = _rows[2];
    row3[fname] = "Coco";
    row3[fage] = 3;
  }
  return _rows;
}

TEST_F(OsqueryJsonTest, basic_add_no_history) {
  std::shared_ptr<vsqlite::ResultsSerializer>  spSerializer = vsqlite::ResultsSerializerNew(OSQUERY_JSON_SERIALIZER_ID);
  auto spListener = std::make_shared<MyDiffResultsListener>();
  std::string historicalData = "";
  spSerializer->beginData(historicalData, spListener, cols);

  auto rows = ExampleData1();

  bool isNewRow = spSerializer->addNewResult(rows[0]);
  EXPECT_TRUE(isNewRow);

  isNewRow = spSerializer->addNewResult(rows[1]);
  EXPECT_TRUE(isNewRow);

  isNewRow = spSerializer->addNewResult(rows[2]);
  EXPECT_TRUE(isNewRow);

  bool hasChanged = spSerializer->endData();
  EXPECT_TRUE(hasChanged);

  ASSERT_EQ(3, spListener->adds.size());
  ASSERT_EQ(0, spListener->removes.size());

  // serialize

  std::string serialized;
  spSerializer->serialize(serialized);
  ASSERT_FALSE(serialized.empty());

  //fprintf(stderr, "'%s'\n", serialized.c_str());

  EXPECT_EQ(gExpected1, serialized);
}

TEST_F(OsqueryJsonTest, basic_add_same_history) {
  std::shared_ptr<vsqlite::ResultsSerializer>  spSerializer = vsqlite::ResultsSerializerNew(OSQUERY_JSON_SERIALIZER_ID);
  auto spListener = std::make_shared<MyDiffResultsListener>();
  spSerializer->beginData(gExpected1, spListener, cols);

  auto rows = ExampleData1();

  bool isNewRow = spSerializer->addNewResult(rows[0]);
  EXPECT_FALSE(isNewRow);

  isNewRow = spSerializer->addNewResult(rows[1]);
  EXPECT_FALSE(isNewRow);

  isNewRow = spSerializer->addNewResult(rows[2]);
  EXPECT_FALSE(isNewRow);

  bool hasChanged = spSerializer->endData();
  EXPECT_FALSE(hasChanged);


  ASSERT_EQ(0, spListener->adds.size());
  ASSERT_EQ(0, spListener->removes.size());

}

TEST_F(OsqueryJsonTest, basic_remove_two) {
  std::shared_ptr<vsqlite::ResultsSerializer>  spSerializer = vsqlite::ResultsSerializerNew(OSQUERY_JSON_SERIALIZER_ID);
  auto spListener = std::make_shared<MyDiffResultsListener>();
  spSerializer->beginData(gExpected1, spListener, cols);

  auto rows = ExampleData1();

  //bool isNewRow = spSerializer->addNewResult(rows[0]);

  bool isNewRow = spSerializer->addNewResult(rows[1]);
  EXPECT_FALSE(isNewRow);

  //isNewRow = spSerializer->addNewResult(rows[2]);

  bool hasChanged = spSerializer->endData();
  EXPECT_TRUE(hasChanged);


  ASSERT_EQ(0, spListener->adds.size());
  ASSERT_EQ(2, spListener->removes.size());

  // serialize

  std::string serialized;
  spSerializer->serialize(serialized);
  ASSERT_FALSE(serialized.empty());

  EXPECT_EQ(gExpected1_row1only, serialized);
}
