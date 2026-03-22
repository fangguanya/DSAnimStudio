from pathlib import Path
import sys

try:
    from tools.formal_validation_lib import ValidationError, load_json, validate_retarget_pose_report, write_json
except ModuleNotFoundError:
    from formal_validation_lib import ValidationError, load_json, validate_retarget_pose_report, write_json


DEFAULT_OUTPUT_DIR = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\HandRetargetDiagnostic")
DEFAULT_INPUT_PATH = DEFAULT_OUTPUT_DIR / "target_retarget_pose_preview.json"
DEFAULT_OUTPUT_PATH = DEFAULT_OUTPUT_DIR / "retarget_pose_validation.json"
DEFAULT_SOURCE_SAMPLES_PATH = DEFAULT_OUTPUT_DIR / "source_animation_samples.json"
DEFAULT_TARGET_SAMPLES_PATH = DEFAULT_OUTPUT_DIR / "target_animation_samples.json"
DEFAULT_ANIMATION_SUMMARY_PATH = DEFAULT_OUTPUT_DIR / "retarget_animation_selftest_summary.json"


def load_optional_animation_reports():
    if not DEFAULT_ANIMATION_SUMMARY_PATH.exists():
        return None, None

    if not DEFAULT_SOURCE_SAMPLES_PATH.exists() or not DEFAULT_TARGET_SAMPLES_PATH.exists():
        return None, None

    return load_json(DEFAULT_SOURCE_SAMPLES_PATH), load_json(DEFAULT_TARGET_SAMPLES_PATH)


def main() -> int:
    input_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_INPUT_PATH
    output_path = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUTPUT_PATH
    report = load_json(input_path)

    source_samples, target_samples = load_optional_animation_reports()

    try:
        summary = validate_retarget_pose_report(report, source_samples, target_samples)
        write_json(output_path, summary)
        print(f"retarget pose validation passed: {output_path}")
        return 0
    except ValidationError as ex:
        failure = {
            "success": False,
            "issues": str(ex).splitlines(),
            "animationSamplesUsed": source_samples is not None and target_samples is not None,
        }
        write_json(output_path, failure)
        print(str(ex), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())