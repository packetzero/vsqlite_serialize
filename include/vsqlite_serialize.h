#pragma once

#include <memory>
#include <string>
#include <dynobj.hpp>

namespace vsqlite {

  template <class T>
  struct DiffResultsListener {

    virtual void onAdded(T &row) = 0;

    virtual void onRemoved(T &row) = 0;
  };

  typedef std::shared_ptr<DiffResultsListener<DynMap> > SPDiffResultsListener;

  typedef std::map<std::string,std::string> StringMap;
  typedef std::shared_ptr<DiffResultsListener<StringMap> > SPDiffResultsListenerStringMap;

  template <class T>
  struct ResultsSerializer {

    /**
     * Initialize with historical data and optional listener.
     * @returns true if unable to parse historical_data.
     */
    virtual bool beginData(std::string &historical_data, std::shared_ptr<DiffResultsListener<T> > listener, std::vector<SPFieldDef> &knownColumnIds) = 0;

    /**
     * If row is not in historical_data, then listener.onAdded()
     * will be called.
     * @returns true if row was not in historical_data.
     */
    virtual bool addNewResult(T &row) = 0;

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

  std::shared_ptr<ResultsSerializer<DynMap> > CrowResultsSerializerNew();
  std::shared_ptr<ResultsSerializer<DynMap> > JsonResultsSerializerNew();
  std::shared_ptr<ResultsSerializer<StringMap> > OsqueryJsonResultsSerializerNew();
}
