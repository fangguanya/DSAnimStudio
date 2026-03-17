using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using SoulsFormats;

namespace DSAnimStudio.Export
{
    /// <summary>
    /// Exports game parameter tables (AtkParam, BehaviorParam, SpEffectParam, EquipParamWeapon)
    /// to structured JSON format.
    /// </summary>
    public class ParamExporter
    {
        public JObject ExportTypedParamTable<T>(IReadOnlyDictionary<long, T> rows, string paramName)
        {
            var result = new JObject
            {
                ["paramName"] = paramName,
                ["rowCount"] = rows?.Count ?? 0,
            };

            var rowsObj = new JObject();
            if (rows != null)
            {
                foreach (var kvp in rows.OrderBy(entry => entry.Key))
                {
                    rowsObj[kvp.Key.ToString()] = SerializeObject(kvp.Value);
                }
            }

            result["rows"] = rowsObj;
            return result;
        }

        public JObject ExportTypedParams(IReadOnlyDictionary<string, JObject> typedTables)
        {
            var result = new JObject();
            foreach (var kvp in typedTables.OrderBy(entry => entry.Key, StringComparer.Ordinal))
            {
                result[kvp.Key] = kvp.Value;
            }

            return result;
        }

        /// <summary>
        /// Export a PARAM file to JSON using reflection on the row data.
        /// </summary>
        public JObject ExportParam(PARAM param, string paramName)
        {
            var result = new JObject();
            result["paramName"] = paramName;
            result["rowCount"] = param.Rows.Count;

            var rowsObj = new JObject();
            foreach (var row in param.Rows)
            {
                var rowObj = new JObject();
                rowObj["id"] = row.ID;
                rowObj["name"] = row.Name ?? "";

                // Serialize all cells
                if (row.Cells != null)
                {
                    var cellsObj = new JObject();
                    foreach (var cell in row.Cells)
                    {
                        try
                        {
                            var cellDef = cell.Def;
                            string cellName = cellDef.InternalName ?? cellDef.DisplayName ?? $"field_{cellDef.SortID}";

                            object val = cell.Value;
                            if (val is bool boolVal)
                                cellsObj[cellName] = boolVal;
                            else if (val is int intVal)
                                cellsObj[cellName] = intVal;
                            else if (val is uint uintVal)
                                cellsObj[cellName] = uintVal;
                            else if (val is short shortVal)
                                cellsObj[cellName] = shortVal;
                            else if (val is ushort ushortVal)
                                cellsObj[cellName] = ushortVal;
                            else if (val is byte byteVal)
                                cellsObj[cellName] = byteVal;
                            else if (val is sbyte sbyteVal)
                                cellsObj[cellName] = sbyteVal;
                            else if (val is float floatVal)
                                cellsObj[cellName] = floatVal;
                            else if (val is double doubleVal)
                                cellsObj[cellName] = doubleVal;
                            else if (val is long longVal)
                                cellsObj[cellName] = longVal;
                            else if (val != null)
                                cellsObj[cellName] = val.ToString();
                        }
                        catch
                        {
                            // Skip problematic cells
                        }
                    }
                    rowObj["cells"] = cellsObj;
                }

                rowsObj[row.ID.ToString()] = rowObj;
            }

            result["rows"] = rowsObj;
            return result;
        }

        /// <summary>
        /// Export multiple param tables to a combined JSON file.
        /// </summary>
        public void ExportParamsToFile(Dictionary<string, PARAM> paramDict, string outputPath)
        {
            var result = new JObject();
            result["version"] = "1.0";
            result["gameType"] = "SDT";

            var paramsObj = new JObject();
            foreach (var kvp in paramDict)
            {
                paramsObj[kvp.Key] = ExportParam(kvp.Value, kvp.Key);
            }

            result["params"] = paramsObj;

            var dir = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);

            File.WriteAllText(outputPath, result.ToString(Formatting.Indented));
        }

        /// <summary>
        /// Export prosthetic tool DummyPoly override configuration.
        /// </summary>
        public JObject ExportProstheticOverrides(
            Dictionary<string, int[]> prostheticOverrides)
        {
            var result = new JObject();
            foreach (var kvp in prostheticOverrides.OrderBy(entry => entry.Key, StringComparer.Ordinal))
            {
                result[kvp.Key] = new JObject
                {
                    ["dummyPolyIds"] = new JArray(kvp.Value ?? Array.Empty<int>()),
                };
            }
            return result;
        }

        private static JToken SerializeObject(object value)
        {
            if (value == null)
                return JValue.CreateNull();

            Type valueType = value.GetType();
            if (valueType.IsEnum)
            {
                return new JObject
                {
                    ["name"] = value.ToString(),
                    ["value"] = Convert.ToInt64(value),
                };
            }

            if (value is string stringValue)
                return stringValue;

            if (value is bool boolValue)
                return boolValue;

            if (value is byte or sbyte or short or ushort or int or uint or long or ulong or float or double or decimal)
                return JToken.FromObject(value);

            if (value is System.Collections.IEnumerable enumerable && value is not string)
            {
                var array = new JArray();
                foreach (var item in enumerable)
                    array.Add(SerializeObject(item));
                return array;
            }

            var obj = new JObject();
            foreach (var field in valueType.GetFields(BindingFlags.Instance | BindingFlags.Public))
            {
                obj[field.Name] = SerializeObject(field.GetValue(value));
            }

            foreach (var property in valueType.GetProperties(BindingFlags.Instance | BindingFlags.Public))
            {
                if (!property.CanRead || property.GetIndexParameters().Length > 0)
                    continue;

                if (obj.ContainsKey(property.Name))
                    continue;

                object propertyValue;
                try
                {
                    propertyValue = property.GetValue(value);
                }
                catch
                {
                    continue;
                }

                obj[property.Name] = SerializeObject(propertyValue);
            }

            return obj;
        }
    }
}
