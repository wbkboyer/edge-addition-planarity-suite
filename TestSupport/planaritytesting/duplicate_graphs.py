import argparse
from os import walk
from pathlib import Path
from shutil import rmtree

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage="python %(prog)s [options]",
        description="Duplicate every graph in each .g6 file in a directory "
        "a given number of times\n\n"
        "When given an input directory, iterate over only the .g6 files under "
        "the directory. For each file, a separate .g6 file:\n"
        "\t{input_dir}-expanded/{input_file.stem}.dup-{duplication_factor}-"
        "times.g6\n"
        "and write every line in the input file {duplication_factor} times to "
        "the output file.",
    )
    parser.add_argument(
        "-i",
        "--inputdir",
        type=Path,
        required=True,
        help="Path to directory contianing .g6 files you wish to expand",
        metavar="INPUT_DIR_PATH",
    )
    parser.add_argument(
        "-d",
        "--duplicationfactor",
        type=int,
        required=False,
        default=10,
        help="How many times do you wish to duplicate every graph in each "
        "infile.",
        metavar="D",
    )

    args = parser.parse_args()

    input_dir = Path(args.inputdir).resolve()
    if not input_dir.is_dir():
        raise ValueError(
            f"input_dir = '{input_dir}' does not correspond to a " "directory."
        )

    duplication_factor = args.duplicationfactor

    output_dir = Path.joinpath(input_dir.parent, input_dir.name + "-expanded")
    rmtree(output_dir, ignore_errors=True)
    Path.mkdir(output_dir, parents=True, exist_ok=True)

    for dirpath, _, filenames in walk(input_dir):
        for filename in filenames:
            input_file_path = Path.joinpath(Path(dirpath), filename)
            if input_file_path.suffix == ".g6":
                output_file_path = Path.joinpath(
                    output_dir,
                    f"{input_file_path.stem}.dup-{duplication_factor}-times.g6",
                )
                with open(input_file_path, "r", encoding="utf-8") as infile:
                    for line in infile:
                        if line:
                            with open(
                                output_file_path, "a", encoding="utf-8"
                            ) as outfile:
                                for _ in range(duplication_factor):
                                    outfile.write(line)
