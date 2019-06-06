#include "../include/vsqlite_serialize.h"

static const SPFieldDef COL_PATH = FieldDef::alloc(TSTRING, "path");
static const SPFieldDef COL_NAME = FieldDef::alloc(TSTRING, "name");
static const SPFieldDef COL_STATE = FieldDef::alloc(TSTRING, "state");
static const SPFieldDef COL_CMDLINE = FieldDef::alloc(TSTRING, "cmdline");
static const SPFieldDef COL_CWD = FieldDef::alloc(TSTRING, "cwd");
static const SPFieldDef COL_ROOT = FieldDef::alloc(TSTRING, "root");

static const SPFieldDef COL_PID = FieldDef::alloc(TINT64, "pid");
static const SPFieldDef COL_UID = FieldDef::alloc(TINT64, "uid");
static const SPFieldDef COL_GID = FieldDef::alloc(TINT64, "gid");
static const SPFieldDef COL_EUID = FieldDef::alloc(TINT64, "euid");
static const SPFieldDef COL_EGID = FieldDef::alloc(TINT64, "egid");
static const SPFieldDef COL_SUID = FieldDef::alloc(TINT64, "suid");
static const SPFieldDef COL_SGID = FieldDef::alloc(TINT64, "sgid");

static const SPFieldDef COL_WIRED_SIZE = FieldDef::alloc(TINT64, "wired_size");
static const SPFieldDef COL_RES_SIZE = FieldDef::alloc(TINT64, "resident_size");
static const SPFieldDef COL_TOTAL_SIZE = FieldDef::alloc(TINT64, "total_size");
static const SPFieldDef COL_USER_TIME = FieldDef::alloc(TINT64, "user_time");
static const SPFieldDef COL_SYS_TIME = FieldDef::alloc(TINT64, "system_time");
static const SPFieldDef COL_START_TIME = FieldDef::alloc(TINT64, "start_time");
static const SPFieldDef COL_PARENT = FieldDef::alloc(TINT64, "parent");
static const SPFieldDef COL_PGROUP = FieldDef::alloc(TINT64, "pgroup");
static const SPFieldDef COL_ELAPSED = FieldDef::alloc(TINT64, "elapsed_time");
static const SPFieldDef COL_HANDLE_COUNT = FieldDef::alloc(TINT64, "handle_count");
//static const SPFieldDef COL_DISK_READ = FieldDef::alloc(TINT64, "disk_bytes_read");
//static const SPFieldDef COL_DISK_WRITE = FieldDef::alloc(TINT64, "disk_bytes_written");

static const SPFieldDef COL_ONDISK = FieldDef::alloc(TINT32, "on_disk");
static const SPFieldDef COL_THREADS = FieldDef::alloc(TINT32, "threads");
static const SPFieldDef COL_ELEVATED = FieldDef::alloc(TINT32, "is_elevated_token");

// in order to be able to consistently traverse rows in same column order...

static std::vector<SPFieldDef> cols = {
  COL_PATH, COL_NAME, COL_STATE, COL_CMDLINE, COL_CWD, COL_ROOT,
  COL_PID, COL_UID, COL_GID, COL_EUID, COL_EGID, COL_SUID, COL_SGID,
  COL_WIRED_SIZE, COL_RES_SIZE, COL_TOTAL_SIZE, COL_USER_TIME, COL_SYS_TIME,
  COL_START_TIME, COL_PARENT, COL_PGROUP, COL_ELAPSED, COL_HANDLE_COUNT,
  COL_ONDISK, COL_THREADS, COL_ELEVATED
};

static std::vector<DynMap> gSourceData;
static std::vector<std::map<std::string,std::string> > gSourceStringMapData;

void generateSourceData(std::vector<DynMap> &dest)
{
  srand (time(NULL));
  for (int i=0; i < dest.size(); i++) {
    auto &row = dest[i];
    int mode = rand() % 3;

    for (auto &col : cols) {
      switch(col->typeId) {
        case TINT64:
          if (mode == 1) {
            row[col] = (int64_t)((uint32_t)rand() % 0x0FFFFFFL);
          } else if (mode == 2 ){
            row[col] = (int64_t)0;
          } else {
            row[col] = (int64_t)-1;
          }
          break;
        case TINT32:
          if (mode == 1) {
            row[col] = (int32_t)((uint32_t)rand() % 0x0FFFFL);
          } else if (mode == 2 ){
            row[col] = (int32_t)0;
          } else {
            row[col] = (int32_t)-1;
          }
          break;
        case TSTRING:
        default:
          if (mode == 1) {
            row[col] = "C:\\Program Files\\Some Application\\Contents\\blah blah blah 3322\\lkjlf20932080480234928590585";
          } else if (mode == 2 ){
            row[col] = "jlajflasjf;laskjf;lsdjflj";
          } else {
            row[col] = "";
          }
          break;
      }
    }
  }
}

void generateSourceData(std::vector<std::map<std::string,std::string>> &dest)
{
  srand (time(NULL));
  for (int i=0; i < dest.size(); i++) {
    auto &row = dest[i];
    int mode = rand() % 3;
    
    for (auto &col : cols) {
      switch(col->typeId) {
        case TINT64:
          if (mode == 1) {
            row[col->name] = std::to_string((int64_t)((uint32_t)rand() % 0x0FFFFFFL));
          } else if (mode == 2 ){
            row[col->name] = "0";
          } else {
            row[col->name] = "-1";
          }
          break;
        case TINT32:
          if (mode == 1) {
            row[col->name] = std::to_string( (int32_t)((uint32_t)rand() % 0x0FFFFL) );
          } else if (mode == 2 ){
            row[col->name] = "0";
          } else {
            row[col->name] = "-1";
          }
          break;
        case TSTRING:
        default:
          if (mode == 1) {
            row[col->name] = "C:\\Program Files\\Some Application\\Contents\\blah blah blah 3322\\lkjlf20932080480234928590585";
          } else if (mode == 2 ){
            row[col->name] = "jlajflasjf;laskjf;lsdjflj";
          } else {
            row[col->name] = "";
          }
          break;
      }
    }
  }
}

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
    SimpleRowToJSONString(row);
  }

  void onRemoved(DynMap &row) override {
    SimpleRowToJSONString(row);
  }
};

void adjustLeftAndRight(int totalRows, int i, int &idxLeft, int &idxRight)
{
  // move left and right indexes
  int stepSize = rand() % 50;
  if (i % 3 == 2) { stepSize *= -1; }
  idxLeft += stepSize / 2;
  idxRight -= stepSize / 2;
  if (idxLeft < 0 || idxLeft >= (totalRows / 4)) idxLeft = 0;
  if (idxRight >= totalRows || idxRight <= (totalRows * 3 / 4)) idxRight = totalRows - 1;
}

void test_dynmap(int totalRows, int iterations, std::shared_ptr<vsqlite::ResultsSerializer<DynMap> > spSerializer)
{
  gSourceData.resize(totalRows);
  generateSourceData(gSourceData);
  auto &data = gSourceData;

  auto spListener = std::make_shared<MyDiffResultsListener>();
  std::string historicalData;

  int idxLeft = 0;
  int idxRight = totalRows - 1;

  for (int i=0; i < iterations; i++) {
    spSerializer->beginData(historicalData, spListener, cols);

    for (int j = idxLeft; j <= idxRight; j++) {

      bool isNewRow = spSerializer->addNewResult(data[j]);
    }

    bool hasChanged = spSerializer->endData();

    historicalData.clear();
    spSerializer->serialize(historicalData);

    adjustLeftAndRight(totalRows, i, idxLeft, idxRight);
  }
}

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
    SimpleRowToJSONString(row);
  }
  
  void onRemoved(Row &row) override {
    SimpleRowToJSONString(row);
  }
};

void test_stringmap(int totalRows, int iterations, std::shared_ptr<vsqlite::ResultsSerializer<std::map<std::string,std::string> > > spSerializer)
{
  gSourceStringMapData.resize(totalRows);
  generateSourceData(gSourceStringMapData);
  auto &data = gSourceStringMapData;
  
  auto spListener = std::make_shared<StringMapDiffResultsListener>();
  std::string historicalData;
  
  int idxLeft = 0;
  int idxRight = totalRows - 1;
  
  for (int i=0; i < iterations; i++) {
    spSerializer->beginData(historicalData, spListener, cols);
    
    for (int j = idxLeft; j <= idxRight; j++) {
      
      bool isNewRow = spSerializer->addNewResult(data[j]);
    }
    
    bool hasChanged = spSerializer->endData();

    historicalData.clear();
    spSerializer->serialize(historicalData);
    
    adjustLeftAndRight(totalRows, i, idxLeft, idxRight);
  }
}

int main(int argc, char *argv[])
{
  int totalRows = 1000;
  int numIterations = 500;
  std::string serializerName;
  if (argc == 4) {
    int total = atoi(argv[1]);
    int count = atoi(argv[2]);
    serializerName = argv[3];
    if (total > 0) totalRows = total;
    if (count > 0) numIterations = count;
  }

  if (serializerName == "json") {

    fprintf(stderr, "Start total:%d iterations:%d serializer:%s\n", totalRows, numIterations, serializerName.c_str());
    auto spSerializer = vsqlite::JsonResultsSerializerNew();
    test_dynmap(totalRows, numIterations, spSerializer);

  } else if (serializerName == "osquery") {

    fprintf(stderr, "Start total:%d iterations:%d serializer:%s\n", totalRows, numIterations, serializerName.c_str());
    auto spSerializer = vsqlite::OsqueryJsonResultsSerializerNew();
    test_stringmap(totalRows, numIterations, spSerializer);

  } else if (serializerName == "stringmapjson") {
    
    fprintf(stderr, "Start total:%d iterations:%d serializer:%s\n", totalRows, numIterations, serializerName.c_str());
    auto spSerializer = vsqlite::JsonStringMapResultsSerializerNew();
    test_stringmap(totalRows, numIterations, spSerializer);
    
  } else {
    fprintf(stderr, "Start total:%d iterations:%d serializer:crow\n", totalRows, numIterations);
    auto spSerializer = vsqlite::CrowResultsSerializerNew();
    test_dynmap(totalRows, numIterations, spSerializer);
  }

  return 0;
}
