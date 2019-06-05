#include <gtest/gtest.h>
#include <string>
#include "../include/vsqlite_serialize.h"
#include "../src/utils.h"

class CrowTest : public ::testing::Test {
protected:
  virtual void SetUp() {  }
};

static const SPFieldDef fname = FieldDef::alloc(TSTRING, "name");
static const SPFieldDef fage = FieldDef::alloc(TINT32, "age");
static const SPFieldDef factive = FieldDef::alloc(TUINT8, "active");

// in order to be able to consistently traverse rows in same column order...

static std::vector<SPFieldDef> cols = { fname, fage, factive };

static const SPFieldDef femoji = FieldDef::alloc(TSTRING, "emoji");

static std::vector<SPFieldDef> cols2 = { fname, fage, factive, femoji };

// this is very simple, no escaping.  Just a way to get a printable value
std::string SimpleRowToJSONString(DynMap &row) {
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


struct MyDiffResultsListener: public vsqlite::DiffResultsListener<DynMap> {
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

static std::string gExpectedHex1 = "43000100046e616d6543010200036167654302090006616374697665" "058003626f6281408201" "0580044a7564798200" "058004436f636f8106";
static std::string gExpectedHex1_row1only = "43000100046e616d6543010200036167654302090006616374697665" "0580044a7564798200";
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

static std::string gExpectedHex2 = "43000100046e616d65430102000361676543020900066163746976654303010005656d6f6a69"
"05800c420000006f00000062000000814082018304f09f9a94"
"0580044a75647982008308f09f9880f09f8cb4"
"058004436f636f81068308f09f8dbaf09f9988";

static std::string gExpectedHex2_row1only =
"43000100046e616d6543010200036167654302090006616374697665" "4303010005656d6f6a69"
 "0580044a75647982008308f09f9880f09f8cb4";

static const std::vector<DynMap> &ExampleData2() {
  static std::vector<DynMap> _rows;
  if (_rows.empty()) {
    _rows.resize(3);
    DynMap &row = _rows[0];

    // create a wide string, copy bytes into regular string
    std::wstring wideBob = L"Bob";
    std::string tmp;
    tmp.resize(wideBob.size() * sizeof(wchar_t));
    memcpy((char *)tmp.data(), wideBob.data(), tmp.size());

    row[fname] = tmp;
    row[femoji] = "🚔";
    row[fage] = 32;
    row[factive] = true;

    DynMap &row2 = _rows[1];
    row2[fname] = "Judy";
    row2[femoji] = "😀🌴";
    row2[fage] = DynVal(); // null / notset
    row2[factive] = false;

    DynMap &row3 = _rows[2];
    row3[fname] = "Coco";
    row3[fage] = 3;
    row3[femoji] = "🍺🙈";
  }
  return _rows;
}


TEST_F(CrowTest, basic_add_no_history) {
  auto spSerializer = vsqlite::CrowResultsSerializerNew();
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
  std::string serializedHex;

  vsqlite_utils::BytesToHexString(serialized, serializedHex);

  //fprintf(stderr, "'%s'\n", serializedHex.c_str());

  EXPECT_EQ(gExpectedHex1, serializedHex);
}

TEST_F(CrowTest, basic_add_same_history) {
  auto spSerializer = vsqlite::CrowResultsSerializerNew();
  auto spListener = std::make_shared<MyDiffResultsListener>();
  std::string historicalData;
  vsqlite_utils::HexStringToBinString(gExpectedHex1, historicalData);
  spSerializer->beginData(historicalData, spListener, cols);

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

TEST_F(CrowTest, basic_remove_two) {
  auto spSerializer = vsqlite::CrowResultsSerializerNew();
  auto spListener = std::make_shared<MyDiffResultsListener>();
  std::string historicalData;
  vsqlite_utils::HexStringToBinString(gExpectedHex1, historicalData);
  spSerializer->beginData(historicalData, spListener, cols);

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
  std::string serializedHex;

  vsqlite_utils::BytesToHexString(serialized, serializedHex);

  EXPECT_EQ(gExpectedHex1_row1only, serializedHex);
}

// make sure wide-characters and special characters are preserved

TEST_F(CrowTest, no_history_special_chars) {
  auto spSerializer = vsqlite::CrowResultsSerializerNew();
  auto spListener = std::make_shared<MyDiffResultsListener>();
  std::string historicalData = "";
  spSerializer->beginData(historicalData, spListener, cols2);

  auto rows = ExampleData2();

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
  std::string serializedHex;

  vsqlite_utils::BytesToHexString(serialized, serializedHex);

  //fprintf(stderr, "'%s'\n", serializedHex.c_str());

  EXPECT_EQ(gExpectedHex2, serializedHex);
}

TEST_F(CrowTest, special_chars_remove_two) {
  auto spSerializer = vsqlite::CrowResultsSerializerNew();
  auto spListener = std::make_shared<MyDiffResultsListener>();
  std::string historicalData;
  vsqlite_utils::HexStringToBinString(gExpectedHex2, historicalData);
  spSerializer->beginData(historicalData, spListener, cols2);
  
  auto rows = ExampleData2();
  
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
  std::string serializedHex;
  
  vsqlite_utils::BytesToHexString(serialized, serializedHex);
  
  EXPECT_EQ(gExpectedHex2_row1only, serializedHex);
}
