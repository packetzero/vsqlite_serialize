# C++ Object serialization layer

This library was designed to be a self-contained module for osquery-like applications to serialize results, determine differential results, and have flexible protocols.

## Protocols

 - **osquery** JSON serialization of StringMap (`std::map<std::string,std::string>`) array
 - **crow** Binary protobuf-like protocol [crow github](https://github.com/packetzero/crow). Uses typed DynMap row objects.
 - **json** A newline delimited list of JSON results. Uses typed DynMap row objects.
 - **stringmapjson** Same as 'json', but row type is `std::map<std::string,std::string>`.

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

The benchmark test uses a 'processes'-like table with 25 columns (see benchmain.cpp).  The generated test data is somewhat random, so the sizes will vary a little bit (5 to 10%) between runs.

<table>
<tr><th>Num Rows</th><th>crow</th><th>json</th><th>stringmap json</th><th>osquery json</th></tr>
<tr><td>100</td><td>28KB</td><td>62KB</td><td>65KB</td><td>63KB</td></tr>
<tr><td>1000</td><td>303KB</td><td>647KB</td><td>652KB</td><td>652KB</td></tr>
</table>

## Performance

The difference between json and stringmap json is that the typed DynMap rows have to be rendered to string values for each iteration, while in the StringMap case, the source data is already in string format, and does not need to be converted for each iteration.
<table>
<tr><th>Num Rows</th><th>Iterations</th><th>crow</th><th>stringmap json</th><th>json</th><th>osquery json</th></tr>
<tr><td>100</td><td>5000</td><td>4.14s</td><td>4.63s</td><td>6.35s</td><td>11.69s</td></tr>
<tr><td>1000</td><td>5000</td><td>25.2s</td><td>43.2s</td><td>54.9s</td><td>124.4s</td></tr>
</table>

## Support for non-ascii data

One of the challenges in osquery is the support for storing non-ascii data.  For example, windows wide-characters, unicode, and some UTF8 characters.  JSON encoding does not support these characters in standard fields, and requires escaping certain characters (quotes, brackets, etc.).  One of the advantages of binary protocols like protobuf and crow is the seamless support of any binary byte data in string fields.

## Differential Algorithms

### Diffs : osquery json encoding

The row format is a `std::map<std::string,std::string>` (aka StringMap).  The data set is an array, so the entire historical dataset needs to be decoded for comparison with the new dataset. The osquery codebase uses a `std::multiset` for convenient lookup. One downside of this approach is that the entire rowset needs to be decoded in memory, rather than one row at a time. Pseudo-code:
```
   jsonObj = JSON.parse(historicalDataString)
   std::multiset histRows;
   fillSetFrom(jsonObj);
   for (rowObj : currentData) {
     if (!histRows.find(rowObj)) {
       notifyAddedRow(rowObj);
     } else {
       histRows.erase(rowObj);
     }
   }
   for (rowObj : histRows) {
     notifyRemovedRow(rowObj);
   }
   // update
   historicalData = JSON.encode(newDataSet)
```

### Diffs : json lines

Rather than keep all rows of a dataset in an array, this approach writes each json row string, separated by newlines.  The row objects are a typed DynMap. The new dataset is encoded into JSON **prior** to differential comparison.  The comparisons are done on the hash of JSON encoded string values in a `std::unordered_set`, rather than the row structures. Pseudo-code:
```
  std::string jsonRowStrings[] = historicalDataString.split('\n');
  std::unordered_set histRows;
  fillSetFrom(jsonRowStrings);
  currentEncodedString = ""
  for (rowObj : currentData) {
    encodedRowString = JSON.encode(rowObj)
    if (!histRows.find(encodedRowString)) {
       notifyAddedRow(rowObj);
       currentEncodedString += encodedRowString + "\n";
    } else {
      histRows.erase(encodedRowString);
    }
  }
  for (encodedRowJson : histRows) {
    notifyRemovedRow(JSON.decode(encodedRowJson));
  }
  // update
  historicalData = currentEncodedString;
```

### Diffs : json lines with StringMap

The algorithm is the same as the json approach, except the row type is a StringMap.

### Diffs : crow encoding

Similar to the json approach above, it does differential comparisons on the hash of encoded row `std::unordered_set`.  Therefore, each new row object is encoded prior to comparison. Pseudo-code:
```
  std::unordered_set histRows = crow.extractEncodedRows(historicalData);
  for (rowObj : currentData) {
    encodedRowString = crow.encode(rowObj)
    if (!histRows.find(encodedRowString)) {
       notifyAddedRow(rowObj);
    } else {
      histRows.erase(encodedRowString);
    }
  }
  for (encodedRowString : histRows) {
    notifyRemovedRows(crow.decode(encodedRowString));
  }
  // update
  historicalData = crow.encodingBuffer;
```
