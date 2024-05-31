#!/usr/bin/env python

__all__ = ['distribute_planarity_workload']

import shutil
import multiprocessing
import subprocess
import argparse
from pathlib import Path


_planarity_algorithm_commands = ('p', 'd', 'o', '2', '3', '4')

def call_planarity(
        planarity_path:Path, canonical_files: bool, command:str, order:int,
        num_edges:int, input_dir:Path, output_dir:Path):
    """Call planarity as blocking process on multiprocessing thread

    Uses subprocess.run() to start a blocking process on the multiprocessing
    pool thread to call the planarity executable so that it applies the
    algorithm specified by the command to all graphs in the input file and
    outputs results to the output file, i.e. calls
       planarity -t {command} {infile_path} {outfile_path}

    Args:
        planarity_path: Path to the planarity executable
        canonical_files: Bool to indicate whether or not the .g6 input files
            are in canonical form
        command: Algorithm specifier character
        order: Desired number of vertices
        num_edges: Desired number of edges
        input_dir: Directory containing the .g6 file with all graphs of the
            given order and num_edges
        output_dir: Directory to which you wish to write the output of applying
            the algorithm corresponding to the command to all graphs in the
            input .g6 file
    """
    canonical_ext = '.canonical' if canonical_files else ''
    infile_path = Path.joinpath(
        input_dir,
        f"n{order}.m{num_edges}{canonical_ext}.g6")
    outfile_path = Path.joinpath(
        output_dir,
        f"{command}",
        f"n{order}.m{num_edges}{canonical_ext}.{command}.out.txt")
    subprocess.run(
        [
            f'{planarity_path}',
            '-t', f'-{command}',
            f'{infile_path}',
            f'{outfile_path}'
        ],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def _validate_planarity_workload_args(
        planarity_path: Path, order: int, input_dir: Path, output_dir: Path
        )->tuple[Path, int, Path, Path]:
    """Validates args provided to distribute_planarity_workload

    Ensures planarity_path corresponds to an executable, that order is an
    integer in closed interval [2, 12], that input_dir is a valid Path that
    corresponds to a directory containing .g6 files, and that output_dir is a
    valid Path specifying where results from executing planarity should be
    output.
    
    Args:
        planarity_path: Path to the planarity executable
        order: Desired number of vertices
        input_dir: Directory containing all graphs of the desired order,
            with each file containing all graphs of a specific edge-count
        output_dir: Directory to which the results of executing planarity Test
            All Graphs for the respective command on each .g6 file will be
            written

    Raises:
        argparse.ArgumentTypeError: If any of the args passed from the command
            line are determined invalid under more specific scrutiny
    
    Returns:
        A tuple comprised of the planarity_path, order, input_dir, and
        output_dir
    """
    if (not planarity_path or
        not isinstance(planarity_path, Path) or
        not shutil.which(str(planarity_path.resolve()))):
        raise argparse.ArgumentTypeError(
            f"Path for planarity executable '{planarity_path}' does not "
            "correspond to an executable.")
    
    if (not order or
        order < 2 or
        order > 12):
        raise argparse.ArgumentTypeError(
            "Graph order must be between 2 and 12.")
    
    if (not input_dir or
        not isinstance(input_dir, Path) or
        input_dir.is_file()):
        raise argparse.ArgumentTypeError(
            "Input directory path is invalid.")
    
    if (input_dir.is_dir() and not any(Path(input_dir).iterdir())):
        raise argparse.ArgumentTypeError("Input dir exists, but is empty.")

    input_dir = input_dir.resolve()
    try:
        candidate_order_from_path = (int)(input_dir.parts[-1])
    except ValueError:
        pass
    except IndexError as e:
        raise argparse.ArgumentTypeError(
            f"Unable to extract parts from "
            "input dir path '{input_dir}'.") from e
    else:
        if candidate_order_from_path != order:
            raise argparse.ArgumentTypeError(
                f"Input directory '{input_dir}' seems to indicate "
                f"graph order should be '{candidate_order_from_path}'"
                f", which does not mach order from command line args "
                f"'{order}'. Please verify your command line args and retry.")

    if (not output_dir or
        not isinstance(output_dir, Path) or
        output_dir.is_file()):
        raise argparse.ArgumentTypeError(
            "Output directory path is invalid.")

    output_dir = output_dir.resolve()
    try:
        candidate_order_from_path = (int)(output_dir.parts[-1])
    except ValueError:
        output_dir = Path.joinpath(output_dir, str(order))
    except IndexError as e:
        raise argparse.ArgumentTypeError(
            f"Unable to extract parts from "
            "output dir path '{output_dir}'.") from e
    else:
        if candidate_order_from_path != order:
            raise argparse.ArgumentTypeError(
                f"Output directory '{output_dir}' seems to indicate "
                f"graph order should be '{candidate_order_from_path}'"
                f", which does not mach order from command line args "
                f"'{order}'. Please verify your command line args and retry.")

    return planarity_path, order, input_dir, output_dir


def distribute_planarity_workload(
        planarity_path: Path, canonical_files: bool, order: int,
        input_dir: Path, output_dir: Path):
    """Use starmap_async on multiprocessing pool to _call_planarity

    Args:
        planarity_path: Path to the planarity executable
        canonical_files: Bool to indicate whether or not the .g6 input files
            are in canonical form
        order: Desired number of vertices
        input_dir: Directory containing all graphs of the desired order,
            with each file containing all graphs of a specific edge-count
        output_dir: Directory in which we will create one subdirectory per
            graph algorithm command, where the results of executing planarity
            Test All Graphs for the respective command on each .g6 file will be
            written
    """
    planarity_path, order, input_dir, output_dir = \
        _validate_planarity_workload_args(
            planarity_path, order, input_dir, output_dir)

    for command in _planarity_algorithm_commands:
        path_to_make = Path.joinpath(output_dir, f'{command}')
        Path.mkdir(path_to_make, parents=True, exist_ok=True)

    call_planarity_args = [
        (
            planarity_path, canonical_files, command, order, num_edges,
            input_dir, output_dir
        )
        for num_edges in range((int)((order * (order - 1)) / 2) + 1)
        for command in _planarity_algorithm_commands
        ]

    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        _ = pool.starmap_async(call_planarity, call_planarity_args)
        pool.close()
        pool.join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage='python %(prog)s [options]',
        description="""Planarity execution orchestrator

Orchestrates calls to planarity's Test All Graphs functionality.

Expects input directory to contain a subdirectory whose name is the order
containing .g6 files to be tested. Each .g6 file contains all graphs of the
given order with a specific number of edges:
    {input_dir}/{order}/n{order}.m{num_edges}.g6

Output files will have paths:
    {output_dir}/{order}/{command}/n{order}.m{num_edges}.{command}.out.txt
""")
    parser.add_argument(
        '-p', '--planaritypath',
        type=Path,
        metavar='PATH_TO_PLANARITY_EXECUTABLE')
    parser.add_argument(
        '-l', '--canonicalfiles',
        action='store_true'
    )
    parser.add_argument(
        '-n', '--order',
        type=int,
        metavar='N',
        default=11)
    parser.add_argument(
        '-i', '--inputdir',
        type=Path,
        metavar='DIR_CONTAINING_G6_FILES')
    parser.add_argument(
        '-o', '--outputdir',
        type=Path,
        metavar='DIR_FOR_RESULTS')

    args = parser.parse_args()

    planarity_path = args.planaritypath
    canonical_files = args.l
    order = args.order
    input_dir = args.inputdir
    output_dir = args.outputdir

    distribute_planarity_workload(
        planarity_path, canonical_files, order, input_dir, output_dir)