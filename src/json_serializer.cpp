#include "../include/vsqlite_serialize.h"

//namespace rapidjson {
//using SizeType = ::std::size_t;
//} // namespace rapidjson

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace rj = rapidjson;

namespace vsqlite {

class JSONResultsSerializer : public ResultsSerializer {
public:
  virtual ~JSONResultsSerializer() {}
  /**
   * Initialize with historical data and optional listener.
   */
  virtual bool beginData(std::string &historical_data, SPDiffResultsListener listener, std::vector<SPFieldDef> &knownColumnIds) override {
    _addCount = 0;
    _removeCount = 0;
    _listener = listener;

    // TODO: parse and prepare lookups
    return false;
  }

  /**
   * If row is not in historical_data, then listener.onAdded()
   * will be called.
   * @returns true if row was not in historical_data.
   */
  virtual bool addNewResult(DynMap &row) override {
    bool wasFoundInHistoricalResults = false;

    std::string row_json;
    _serializeRow(row, row_json);

    if (!wasFoundInHistoricalResults && _listener) {
      _listener->onAdded(row);
    }
    return !wasFoundInHistoricalResults;
  }

  /**
   * Indicates that all addNewResult() calls have been made for
   * current data set.
   * @return true if new results are different from historical_data,
   * false if unchanged.
   */
  virtual bool endData() override {
    // TODO: find all removed entries
    return _addCount == 0 && _removeCount == 0;
  }

  /**
   * Serializes the current data snapshot into dest.
   */
  virtual void serialize(std::string &dest) override {
    rj::Document doc;
    
  }

protected:
  inline rj::Value SVAL(const std::string &str) {
    rj::Value retval;
    retval.SetString(str.c_str(), str.size(), doc.GetAllocator());
    return retval;
  }

  void _serializeRow(DynMap &row, std::string &dest) {
    rj::Value obj(rj::kObjectType);
    for (auto &it : row) {
      if (!it.second.valid()) {
        continue;
      }
      obj.AddMember(SVAL(it.first->name),SVAL(it.second.as_s()),doc.GetAllocator());
    }
    // render to string
    
    rj::StringBuffer buffer;
    rj::Writer<rj::StringBuffer> writer(buffer);
    obj.Accept(writer);

    dest = buffer.GetString();
  }

  // members
  uint32_t _addCount { 0 };
  uint32_t _removeCount { 0 };
  SPDiffResultsListener _listener;
  rj::Document doc;
};

  std::shared_ptr<ResultsSerializer> JSONResultsSerializerNew() {
    return std::make_shared<JSONResultsSerializer>();
  }

  extern std::shared_ptr<ResultsSerializer> CrowResultsSerializerNew();

  std::shared_ptr<ResultsSerializer> ResultsSerializerNew(char id) {
    switch(id) {
      case JSON_SERIALIZER_ID: return JSONResultsSerializerNew();
      case CROW_SERIALIZER_ID: return CrowResultsSerializerNew();
      default:
        break;
    }
    return nullptr;
  }
} // namespace vsqlite
