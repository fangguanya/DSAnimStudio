from pathlib import Path
import sys

try:
    from tools.formal_validation_lib import ValidationError, load_json, validate_retarget_animation_samples, write_json
except ModuleNotFoundError:
    from formal_validation_lib import ValidationError, load_json, validate_retarget_animation_samples, write_json


DEFAULT_OUTPUT_DIR = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\RetargetAnimationSelfTest")
DEFAULT_SOURCE_SAMPLES_PATH = DEFAULT_OUTPUT_DIR / "source_animation_samples.json"
DEFAULT_TARGET_SAMPLES_PATH = DEFAULT_OUTPUT_DIR / "target_animation_samples.json"
DEFAULT_OUTPUT_PATH = DEFAULT_OUTPUT_DIR / "retarget_animation_validation.json"


def main() -> int:
    source_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_SOURCE_SAMPLES_PATH
    target_path = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_TARGET_SAMPLES_PATH
    output_path = Path(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_OUTPUT_PATH

    source_samples = load_json(source_path)
    target_samples = load_json(target_path)

    try:
        summary = validate_retarget_animation_samples(source_samples, target_samples)
        write_json(output_path, summary)
        print(f"retarget animation validation passed: {output_path}")
        return 0
    except ValidationError as ex:
        failure = {
            "success": False,
            "issues": str(ex).splitlines(),
        }
        write_json(output_path, failure)
        print(str(ex), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())