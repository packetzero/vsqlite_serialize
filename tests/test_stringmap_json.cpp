#include <gtest/gtest.h>
#include <string>

#include "../include/vsqlite_serialize.h"
#include "../src/utils.h"

class StringMapJsonTest : public ::testing::Test {
protected:
  virtual void SetUp() {  }
};

typedef vsqlite::StringMap Row;

// this is very simple, no escaping.  Just a way to get a printable value
static std::string SimpleRowToJSONString( Row &row) {
  std::string s;
  int i=-1;
  for (auto &it : row) {
    i++;

    if (!s.empty()) { s += ", "; }

    s += it.first + ":";
    s += "\"" + it.second + "\"";
  }
  return "{" + s + "}";
}


struct StringMapDiffResultsListener: public vsqlite::DiffResultsListener<Row > {
  virtual ~StringMapDiffResultsListener() {}

  void onAdded(Row &row) override {
    adds.push_back(SimpleRowToJSONString(row));
  }

  void onRemoved(Row &row) override {
    removes.push_back(SimpleRowToJSONString(row));
  }
  std::vector<std::string> adds;
  std::vector<std::string> removes;
};

static std::string gExpected1 = "{\"active\":\"1\",\"age\":\"32\",\"name\":\"bob\"}\n{\"active\":\"0\",\"name\":\"Judy\"}\n{\"age\":\"3\",\"name\":\"Coco\"}\n";

static std::string gExpected1_row1only = "{\"active\":\"0\",\"name\":\"Judy\"}\n";

static std::vector<Row> &ExampleData1() {
  static std::vector<Row> _rows;
  if (_rows.empty()) {
    _rows.resize(3);
    auto &row = _rows[0];
    row["name"] = "bob";
    row["age"] = "32";
    row["active"] = "1";

    auto &row2 = _rows[1];
    row2["name"] = "Judy";
    row2["active"] = "0";

    auto &row3 = _rows[2];
    row3["name"] = "Coco";
    row3["age"] = "3";
  }
  return _rows;
}

TEST_F(StringMapJsonTest, basic_add_no_history) {
  auto spSerializer = vsqlite::JsonStringMapResultsSerializerNew();
  auto spListener = std::make_shared<StringMapDiffResultsListener>();
  std::string historicalData = "";
  std::vector<SPFieldDef> cols;
  spSerializer->beginData(historicalData, spListener, cols);

  auto& rows = ExampleData1();

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

TEST_F(StringMapJsonTest, basic_add_same_history) {
  auto spSerializer = vsqlite::JsonStringMapResultsSerializerNew();
  auto spListener = std::make_shared<StringMapDiffResultsListener>();
  std::vector<SPFieldDef> cols;

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

TEST_F(StringMapJsonTest, basic_remove_two) {
  auto spSerializer = vsqlite::JsonStringMapResultsSerializerNew();
  auto spListener = std::make_shared<StringMapDiffResultsListener>();
  std::vector<SPFieldDef> cols;
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
