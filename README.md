# C++ Object serialization layer

This library was designed to be a self-contained module for osquery-like applications to serialize results, determine differential results, and have flexible protocols.

## Protocols

 - **osquery** JSON serialization of stringmap array, using std::multiset for differential results
 - **crow** Binary protobuf-like protocol, deserialization only on removed rows.
 - **json** A newline delimited list of JSON results, deserialization only on removed rows.

## API

```
// create serializer Crow, Json, or OsqueryJson

auto spSerializer = vsqlite::CrowResultsSerializerNew();
auto spListener = std::make_shared<MyDiffResultsListener>();

// historicalData is previous data set

spSerializer->beginData(historicalData, spListener, cols);

// provide current dataset rows

for (auto & row : rows) {
  bool isNewRow = spSerializer->addNewResult(rows[0]);

}
bool hasChanged = spSerializer->endData();

if (hasChanged) {
  // serialize new data set

  historicalData.clear();
  spSerializer->serialize(historicalData);
}
```

## Storage Size

For 1000 rows of a 'processes'-like table with 25 columns (see benchmain.cpp),
The Json and OsqueryJson serialization was 652 KB, while the crow serialization was 303 KB.

## Performance

For the 1000 rows of data mentioned above, 5000 iterations:
- crow : 25.2 seconds
- json : 54.9 seconds
- osquery json : 124.4 seconds
