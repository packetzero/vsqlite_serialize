#pragma once

#include <memory>
#include <string>
#include <dynobj.hpp>

namespace vsqlite {

  struct DiffResultsListener {

    virtual void onAdded(DynMap &row) = 0;

    virtual void onRemoved(DynMap &row) = 0;
  };

  typedef std::shared_ptr<DiffResultsListener> SPDiffResultsListener;

  struct ResultsSerializer {

    /**
     * Initialize with historical data and optional listener.
     * @returns true if unable to parse historical_data.
     */
    virtual bool beginData(std::string &historical_data, SPDiffResultsListener listener, std::vector<SPFieldDef> &knownColumnIds) = 0;

    /**
     * If row is not in historical_data, then listener.onAdded()
     * will be called.
     * @returns true if row was not in historical_data.
     */
    virtual bool addNewResult(DynMap &row) = 0;

    /**
     * Indicates that all addNewResult() calls have been made for
     * current data set.
     * @return true if new results are different from historical_data,
     * false if unchanged.
     */
    virtual bool endData() = 0;

    /**
     * Serializes the current data snapshot into dest.
     */
    virtual void serialize(std::string &dest) = 0;
  };

#define CROW_SERIALIZER_ID 'c'
#define JSON_SERIALIZER_ID 'j'
#define OSQUERY_JSON_SERIALIZER_ID 'o'

  /*
   * @param id 'c' for crow, 'j' for json
   */
  std::shared_ptr<ResultsSerializer> ResultsSerializerNew(char id = JSON_SERIALIZER_ID);
}
