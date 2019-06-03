#include "../include/vsqlite_serialize.h"

#include <crow.hpp>
#include <crow/crow_decode.hpp>
#include <unordered_map>

#include "utils.h"


#define CHECK_COL(colId) if (nullptr == colId) { assert(false); return ; }

static const SPFieldDef COLISFOUND = FieldDef::alloc(TUINT8, "__isfound");

namespace vsqlite {

  class MyDecoderListener : public crow::DecoderListener {
  public:

    virtual ~MyDecoderListener() {
    }
    
    MyDecoderListener(std::vector<SPFieldDef> &colIds) : crow::DecoderListener(), _rownum(0), _rows(), _colIds(colIds), _encodedRows() {
    }

    virtual void onField(crow::SPCFieldInfo fieldDef, int8_t value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    virtual void onField(crow::SPCFieldInfo fieldDef, uint8_t value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    
    void onField(crow::SPCFieldInfo fieldDef, int32_t value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    void onField(crow::SPCFieldInfo fieldDef, uint32_t value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    void onField(crow::SPCFieldInfo fieldDef, int64_t value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    void onField(crow::SPCFieldInfo fieldDef, uint64_t value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    void onField(crow::SPCFieldInfo fieldDef, double value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    void onField(crow::SPCFieldInfo fieldDef, const std::string &value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }
    void onField(crow::SPCFieldInfo fieldDef, const std::vector<uint8_t> value, uint8_t flags) override {
      SPFieldDef colId = getAppField(fieldDef);
      CHECK_COL(colId);
      getRow()[colId] = DynVal(value);
    }

    DynMap &getRow() {
      // add row if needed
      if (_rows.size() <= _rownum) {
        _rows.push_back(DynMap());
      }
      return _rows[_rows.size()-1];
    }
    
    /*
     * The decoder creates field infos as it decodes, and they
     * won't be the exact same instances as the application uses.
     * This maps them to application field so that comparisons work.
     */
    SPFieldDef getAppField(crow::SPCFieldInfo fieldInfo) {
      for (SPFieldDef colId : _colIds) {
        if (colId->name == fieldInfo->name) {
          return colId;
        }
      }
      return nullptr;
    }

    /*
     * Notifies when a row is finished.
     *
     * For applications doing differential comparisons with
     * new vs persisted data, pEncodedRowStart,length are included.
     */
    virtual void onRowEnd(const uint8_t* pEncodedRowStart, size_t length) override {
      std::string encodedRow;
      encodedRow.append((const char *)pEncodedRowStart, length);
      std::string encodedRowHex;
      vsqlite_utils::BytesToHexString(encodedRow, encodedRowHex);
      _encodedRows[encodedRow] = _rownum;
      _rownum++;
    }

    size_t _rownum;
    std::vector<DynMap > _rows;
    std::vector<SPFieldDef> &_colIds;
    std::unordered_map<std::string, size_t> _encodedRows;
  };

class CrowResultsSerializer : public ResultsSerializer {
public:
  virtual ~CrowResultsSerializer() {
    if (nullptr != _pEnc) { delete _pEnc; }
  }

  /**
   * Initialize with historical data and optional listener.
   */
  virtual bool beginData(std::string &historical_data, SPDiffResultsListener listener, std::vector<SPFieldDef> &knownColumnIds) override {
    _addCount = 0;
    _removeCount = 0;
    _listener = listener;
    if (nullptr != _pEnc) { delete _pEnc; }

    _pEnc = crow::EncoderFactory::New();
    _colIds.clear();

    // add known columns
    if (!knownColumnIds.empty()) {
      for (auto &id : knownColumnIds) { _colIds.push_back(id); }
    }

    // handle historical data
    if (!historical_data.empty()) {
      _decoderListener = std::make_shared<MyDecoderListener>(_colIds);

      crow::Decoder *_pDec = crow::DecoderFactory::New((const uint8_t*)historical_data.data(), historical_data.size());
      _pDec->decode(*_decoderListener.get());
      delete _pDec;
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

    // encode each field

    for (auto &id : _colIds) {
      _pEnc->put(id, row[id]);
    }

    // flush : first headers only, then data

    _pEnc->flush(true); // headers only
    size_t pos = _pEnc->size();
    _pEnc->flush();

    // get encoded row data

    const uint8_t *p = _pEnc->data() + pos + 1; // skip row 0x05 marker
    size_t rowLen = _pEnc->size() - pos - 1;

    // lookup encoded bytes in historical data

    wasFoundInHistoricalResults = _lookupEncodedRow(p, rowLen);

    // notify listener

    if (!wasFoundInHistoricalResults) {
      _addCount++;
      if (_listener) {
        _listener->onAdded(row);
      }
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
    if (nullptr != _decoderListener  && _listener != nullptr) {
      for (auto & row : _decoderListener->_rows) {
        if (row.count(COLISFOUND) == 0) {
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
    _pEnc->flush();
    dest.append((const char *)_pEnc->data(), _pEnc->size());
  }

protected:

  bool _lookupEncodedRow(const uint8_t* ptr, size_t len) {
    if (nullptr == _decoderListener || _decoderListener->_encodedRows.empty()) { return false; }

    std::string key;
    key.append((const char *)ptr, len);
    auto fit = _decoderListener->_encodedRows.find(key);
    if (fit != _decoderListener->_encodedRows.end()) {
      size_t rowIndex = fit->second;
      if (rowIndex < 0 || rowIndex >= _decoderListener->_rows.size()) {
        assert(false);
      } else {
        _decoderListener->_rows[rowIndex][COLISFOUND] = true;
      }
      return true;
    }

    return false;
  }

  // members
  uint32_t _addCount { 0 };
  uint32_t _removeCount { 0 };
  SPDiffResultsListener _listener;
  crow::Encoder *_pEnc {nullptr};
  std::vector<SPFieldDef> _colIds;
  std::shared_ptr<MyDecoderListener> _decoderListener;
};

  std::shared_ptr<ResultsSerializer> CrowResultsSerializerNew() {
    return std::make_shared<CrowResultsSerializer>();
  }

}
