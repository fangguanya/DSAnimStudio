from pathlib import Path
import sys

try:
    from tools.formal_validation_lib import ValidationError, load_json, validate_ue_import_report, write_json
except ModuleNotFoundError:
    from formal_validation_lib import ValidationError, load_json, validate_ue_import_report, write_json


DEFAULT_INPUT_PATH = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\Export_17_Preview\ue_import_report.json")
DEFAULT_OUTPUT_PATH = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\Export_17_Preview\ue_import_validation.json")


def main() -> int:
    input_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_INPUT_PATH
    output_path = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUTPUT_PATH
    report = load_json(input_path)

    try:
        summary = validate_ue_import_report(report)
        write_json(output_path, summary)
        print(f"UE import report validation passed: {output_path}")
        return 0
    except ValidationError as ex:
        failure = {"success": False, "issues": str(ex).splitlines()}
        write_json(output_path, failure)
        print(str(ex), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())