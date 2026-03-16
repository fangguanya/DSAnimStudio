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
            foreach (var kvp in prostheticOverrides)
            {
                result[kvp.Key] = new JArray(kvp.Value);
            }
            return result;
        }
    }
}
