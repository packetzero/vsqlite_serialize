#include "../include/vsqlite_serialize.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <set>

namespace rj = rapidjson;


namespace vsqlite {

typedef std::map<std::string,std::string> Row;

class OsqueryResultsSerializer : public ResultsSerializer {
public:
  virtual ~OsqueryResultsSerializer() {}
  /**
   * Initialize with historical data and optional listener.
   */
  virtual bool beginData(std::string &historical_data, SPDiffResultsListener listener, std::vector<SPFieldDef> &knownColumnIds) override {
    _addCount = 0;
    _removeCount = 0;
    _listener = listener;

    _colIds.clear();

    // add known columns
    if (!knownColumnIds.empty()) {
      for (auto &id : knownColumnIds) { _colIds.push_back(id); }
    }

    if (!historical_data.empty()) {
      _decodeRowArray(historical_data);
    }

    return false;
  }

  /**
   * If row is not in historical_data, then listener.onAdded()
   * will be called.
   * @returns true if row was not in historical_data.
   */
  virtual bool addNewResult(DynMap &row) override {
    bool wasFoundInHistoricalResults = false;

    // get column ids if not set

    if (_colIds.empty()) {
      for (auto &it : row) {
        _colIds.push_back(it.first);
      }
    }

    // convert to stringmap

    Row r;
    for (auto &it : row) {
      if (!it.second.valid()) { continue; }
      r[it.first->name] = it.second.as_s();
    }

    // lookup
    
    auto fit = _prevRows.find(r);
    if (fit != _prevRows.end()) {
      wasFoundInHistoricalResults = true;
      _prevRows.erase(fit);
    }

    if (!wasFoundInHistoricalResults) {
      if (_listener) {
        _listener->onAdded(row);
      }
      _addedRows.push_back(r);
      _addCount++;
    }

    _results.push_back(r);

    return !wasFoundInHistoricalResults;
  }

  /**
   * Indicates that all addNewResult() calls have been made for
   * current data set.
   * @return true if new results are different from historical_data,
   * false if unchanged.
   */
  virtual bool endData() override {
    if (_listener && !_prevRows.empty()) {
      for (auto &row : _prevRows) {
        DynMap tmp;
        _listener->onRemoved(tmp); // TODO: pass templatized row
          _removeCount++;
      }
    }
    // TODO: find all removed entries
    return !(_addCount == 0 && _removeCount == 0);
  }

  /**
   * Serializes the current data snapshot into dest.
   */
  virtual void serialize(std::string &dest) override {
    doc.SetArray();
    for (auto &row : _results) {
      _serializeRow(doc, row);
    }
    
    // render to string
    
    rj::StringBuffer buffer;
    rj::Writer<rj::StringBuffer> writer(buffer);
    doc.Accept(writer);

    dest = buffer.GetString();
  }

protected:
  
  bool deserializeRow(const rj::Value& doc, Row& r) {
    if (!doc.IsObject()) {
      return true;
    }
    
    for (const auto& i : doc.GetObject()) {
      std::string name(i.name.GetString());
      if (!name.empty() && i.value.IsString()) {
        r[name] = i.value.GetString();
      }
    }
    return false;
  }
  
  bool _decodeRowArray(std::string &json) {
    rj::Document doc;
    
    if (doc.Parse(json.c_str()).HasParseError()) {
      // TODO: log
      return true;
    }
    if (!doc.IsArray()) {
      return true;
    }
    for (const auto& i : doc.GetArray()) {
      Row r;
      auto status = deserializeRow(i, r);
      if (status) {
        return status;
      }
      _prevRows.insert(std::move(r));
    }
    return false;

  }

  /*
   * The decoder creates field infos as it decodes, and they
   * won't be the exact same instances as the application uses.
   * This maps them to application field so that comparisons work.
   */
  SPFieldDef GetAppColumnId(std::string fieldName) {
    for (SPFieldDef colId : _colIds) {
      if (colId->name == fieldName) {
        return colId;
      }
    }
    return nullptr;
  }


  inline rj::Value SVAL(const std::string &str) {
    rj::Value retval;
    retval.SetString(str.c_str(), str.size(), doc.GetAllocator());
    return retval;
  }

  void _serializeRow(rj::Value &arr, DynMap &row) {
    rj::Value obj(rj::kObjectType);

    for (auto &id : _colIds) {

      if (!row[id].valid()) {
        continue;
      }
      obj.AddMember(SVAL(id->name),SVAL(row[id].as_s()),doc.GetAllocator());
    }

    arr.PushBack(obj, doc.GetAllocator());
  }

  void _serializeRow(rj::Value &arr, Row &row) {
    rj::Value obj(rj::kObjectType);
    
    for (auto &it : row) {
      obj.AddMember(SVAL(it.first),SVAL(it.second),doc.GetAllocator());
    }
    arr.PushBack(rj::Value(obj, doc.GetAllocator()).Move(),
                 doc.GetAllocator());
    //arr.PushBack(obj, doc.GetAllocator());
  }

  // members
  uint32_t _addCount { 0 };
  uint32_t _removeCount { 0 };
  SPDiffResultsListener _listener;
  std::vector<SPFieldDef> _colIds;
  rj::Document doc;
  std::stringstream _ss;
  std::multiset<Row> _prevRows;
  std::vector<Row> _results;
  std::vector<Row> _addedRows;
  std::vector<Row> _removedRows;
};

  std::shared_ptr<ResultsSerializer> OsqueryResultsSerializerNew() {
    return std::make_shared<OsqueryResultsSerializer>();
  }

} // namespace vsqlite
