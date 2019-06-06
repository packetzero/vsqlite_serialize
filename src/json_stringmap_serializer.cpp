#include "../include/vsqlite_serialize.h"

//namespace rapidjson {
//using SizeType = ::std::size_t;
//} // namespace rapidjson

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <unordered_set>

namespace rj = rapidjson;

static void SPLIT(const std::string &s, char delim, std::unordered_set<std::string> &dest) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    if (item.empty()) { continue; }
    if (item[0] == ' ') { item.erase(0); }
    if (item[item.size()-1] == ' ') { item.erase(item.size()-1); }
    if (item.empty()) { continue; }
    dest.insert(item);
  }
}

namespace vsqlite {

class JsonStringMapResultsSerializer : public ResultsSerializer<StringMap> {
public:
  virtual ~JsonStringMapResultsSerializer() {}
  /**
   * Initialize with historical data and optional listener.
   */
  virtual bool beginData(std::string &historical_data, SPDiffResultsListenerStringMap listener, std::vector<SPFieldDef> &knownColumnIds) override {
    _addCount = 0;
    _removeCount = 0;
    _listener = listener;
    _encodedLines.clear();

//    _colIds.clear();
    _ss = std::stringstream();

    if (!historical_data.empty()) {
      SPLIT(historical_data, '\n', _encodedLines);
    }

    return false;
  }

  /**
   * If row is not in historical_data, then listener.onAdded()
   * will be called.
   * @returns true if row was not in historical_data.
   */
  virtual bool addNewResult(StringMap &row) override {
    bool wasFoundInHistoricalResults = false;

    std::string row_json;
    _serializeRow(row, row_json);

    // append to running encoding
    _ss << row_json << "\n";

    // lookup

    auto fit = _encodedLines.find(row_json);
    if (fit != _encodedLines.end()) {
      wasFoundInHistoricalResults = true;
      _encodedLines.erase(fit);
    }

    if (!wasFoundInHistoricalResults && _listener) {
      _addCount++;
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
    if (_listener && !_encodedLines.empty()) {
      for (auto &encLine : _encodedLines) {
        StringMap row;
        if (_decodeRow(encLine, row)) {
          // fail
        } else {
          _listener->onRemoved(row);
          _removeCount++;
        }
      }
    }
    // TODO: find all removed entries
    return !(_addCount == 0 && _removeCount == 0);
  }

  /**
   * Serializes the current data snapshot into dest.
   */
  virtual void serialize(std::string &dest) override {
    dest = _ss.str();
  }

protected:

  /*
   * return true on error, false on success
   */
  bool _decodeRow(const std::string &encRow, StringMap &row) {
    rj::Document doc;
    if (doc.Parse(encRow.c_str()).HasParseError()) {
      // TODO: log
      return true;
    }
    if (!doc.IsObject()) {
      return true;
    }

    for (const auto& i : doc.GetObject()) {
      std::string name(i.name.GetString());
      if (!name.empty() && i.value.IsString()) {
         row[name] = i.value.GetString();
      }
    }
    return false;
  }

  inline rj::Value SVAL(const std::string &str) {
    rj::Value retval;
    retval.SetString(str.c_str(), str.size(), doc.GetAllocator());
    return retval;
  }

  void _serializeRow(StringMap &row, std::string &dest) {
    rj::Value obj(rj::kObjectType);

    for (auto &id : row) {

      obj.AddMember(SVAL(id.first),SVAL(id.second),doc.GetAllocator());
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
  SPDiffResultsListenerStringMap _listener;
//  std::vector<SPFieldDef> _colIds;
  rj::Document doc;
  std::stringstream _ss;
  std::unordered_set<std::string> _encodedLines;
};

  std::shared_ptr<ResultsSerializer<StringMap> > JsonStringMapResultsSerializerNew() {
    return std::make_shared<JsonStringMapResultsSerializer>();
  }

} // namespace vsqlite
