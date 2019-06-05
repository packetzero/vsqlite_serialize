#include "../include/vsqlite_serialize.h"

#include <crow.hpp>
#include <crow/crow_decode.hpp>
#include <unordered_set>

#include "utils.h"


#define CHECK_COL(colId) if (nullptr == colId) { assert(false); return ; }

static const SPFieldDef COLISFOUND = FieldDef::alloc(TUINT8, "__isfound");

namespace vsqlite {

  /*
   * This decoder listener just extracts encoded rows, not the decoded rows.
   * It's the first pass.
   */
  class RawRowsDecoderListener : public crow::DecoderListener {
  public:

    virtual ~RawRowsDecoderListener() {
    }
    
    RawRowsDecoderListener(std::unordered_set<std::string> &encodedRows, std::string &encodedHeaderRow) : crow::DecoderListener(), _rownum(0), _encodedRows(encodedRows), _encodedHeaderRow(encodedHeaderRow) {
    }

    void onRowEnd(bool isHeaderRow, const uint8_t* pEncodedRowStart, size_t length) override {
      std::string encodedRow;
      encodedRow.append((const char *)pEncodedRowStart, length);
      if (isHeaderRow) {
        _encodedHeaderRow = encodedRow;
      } else {
        _encodedRows.insert(encodedRow);
        _rownum++;
      }
    }

    size_t _rownum;
    std::unordered_set<std::string> &_encodedRows;
    std::string &_encodedHeaderRow;
  };

  /*
   * This decoder listener reassembles rows.
   * It's used in the second pass on the portion of historical data
   * containing removed rows only.
   */
  class MyRowDecoderListener : public crow::DecoderListener {
  public:
    
    virtual ~MyRowDecoderListener() {
    }
    
    MyRowDecoderListener(std::vector<SPFieldDef> &colIds) : crow::DecoderListener(), _rownum(0), _rows(), _colIds(colIds)  {
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
    
    void onRowEnd(bool isHeaderRow, const uint8_t* pEncodedRowStart, size_t length) override {
      if (!isHeaderRow) {
        _rownum++;
      }
    }
    
    size_t _rownum;
    std::vector<DynMap > _rows;
    std::vector<SPFieldDef> &_colIds;
  };

  
class CrowResultsSerializer : public ResultsSerializer<DynMap> {
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

    // extract encoded rows data from historical_data.
    // This is kind of like doing a historical_data.split(\n)

    if (!historical_data.empty()) {
      RawRowsDecoderListener decoderListener(_histEncodedRows, _histEncodedHeaderRow);

      crow::Decoder *_pDec = crow::DecoderFactory::New((const uint8_t*)historical_data.data(), historical_data.size());
      _pDec->setModeFlags(DECODER_MODE_SKIP);
      _pDec->decode(decoderListener);
      _histTotalRows = decoderListener._rownum;

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
    if (_listener != nullptr && !_histEncodedRows.empty()) {
      _decodeAndNotifyRemovedRows();
    }

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

  /*
   * Assembles header + rows[] in dest binary string
   */
  void assembleOnlyRemovedRows(std::string &encodedHeaderRow, std::unordered_set<std::string> &encodedRows, std::vector<uint8_t> &dest) {

    // first determine new size

    size_t len = encodedHeaderRow.size();
    for (auto & encodedRow : encodedRows) {
      len += encodedRow.size() + 1;
    }

    // allocate

    dest.resize(len);
    uint8_t* p = dest.data();

    // add header row
    memcpy(p, encodedHeaderRow.data(), encodedHeaderRow.size());
    p += encodedHeaderRow.size();

    // add each removed row

    for (auto & encodedRow : encodedRows) {
      *p++ = (uint8_t)TROW;
      memcpy(p, encodedRow.data(), encodedRow.size());
      p += encodedRow.size();
    }
  }

  /*
   * Will reassemble parts of historical_data from beginData()
   */
  void _decodeAndNotifyRemovedRows() {
    std::vector<uint8_t> encodedData;
    MyRowDecoderListener listener(_colIds);

    assembleOnlyRemovedRows(_histEncodedHeaderRow, _histEncodedRows, encodedData);

    crow::Decoder *pDec = crow::DecoderFactory::New(encodedData.data(), encodedData.size());

    pDec->decode(listener);

    for (auto & row : listener._rows) {

        _listener->onRemoved(row);
        _removeCount++;
    }

    delete pDec;
  }
  
  bool _lookupEncodedRow(const uint8_t* ptr, size_t len) {
    if (_histEncodedRows.empty()) { return false; }

    std::string key;
    key.append((const char *)ptr, len);
    auto fit = _histEncodedRows.find(key);
    if (fit != _histEncodedRows.end()) {
      _histEncodedRows.erase(fit);

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

  std::unordered_set<std::string> _histEncodedRows;
  std::string _histEncodedHeaderRow;
  size_t _histTotalRows;
};

  std::shared_ptr<ResultsSerializer<DynMap> > CrowResultsSerializerNew() {
    return std::make_shared<CrowResultsSerializer>();
  }

}
