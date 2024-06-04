#!/usr/bin/env python

__all__ = ['distribute_geng_workload']

import sys
import multiprocessing
import subprocess
import argparse
import shutil
from pathlib import Path

from planarity_constants import max_num_edges_for_order

def _call_geng(
        geng_path:Path, canonical_files: bool, order:int, num_edges:int,
        output_dir:Path):
    """Call nauty geng as blocking process on multiprocessing thread

    Opens a file for write (overwrites file if it exists) within the output_dir
    and uses subprocess.run() to start a blocking process on the
    multiprocessing pool thread to call the nauty geng executable with the
    desired order and number of edges, with stdout redirected to the output
    file object.

    The resulting .g6 output file will contain all graphs of the desired order
    for a single edge count.

    Args:
        geng_path: Path to the nauty geng executable
        canonical_files: Bool to indicate whether or not to generate
            graphs with canonical labelling
        order: Desired number of vertices
        num_edges: Desired number of edges
        output_dir: Directory to which you wish to write the resulting .g6 file
    """
    filename = Path.joinpath(
        output_dir,
        f"n{order}.m{num_edges}{'.canonical' if canonical_files else ''}.g6"
        )
    with open(filename, "w") as outfile:
        command = [f'{geng_path}', f'{order}', f'{num_edges}:{num_edges}']
        if canonical_files:
            command.insert(1, '-l')
        subprocess.run(command, stdout=outfile, stderr=subprocess.PIPE)


def _validate_and_normalize_geng_workload_args(
        geng_path: Path, order: int, output_dir: Path)->tuple[Path, int, Path]:
    """Validates and normalizes args provided to distribute_geng_workload

    Ensures geng_path corresponds to an executable, that order is an integer
    in closed interval [2, 12], and that output_dir is a valid Path specifying
    where results from executing geng should be output.
    
    Args:
        geng_path: Path to the nauty geng executable
        order: Desired number of vertices
        output_dir: Directory to which you wish to write the resulting .g6 file
            If none provided, defaults to:
                TestSupport/results/graph_generation_orchestrator/{order}

    Raises:
        argparse.ArgumentTypeError: If any of the args passed from the command
            line are determined invalid under more specific scrutiny
    Returns:
        A tuple comprised of the geng_path, order, and output_dir
    """
    if (not geng_path or
        not isinstance(geng_path, Path) or
        not shutil.which(str(geng_path.resolve()))):
        raise argparse.ArgumentTypeError(
            f"Path for geng executable '{geng_path}' does not correspond to "
            "an executable.")
    
    if (not order or
        order < 2 or
        order > 12):
        raise argparse.ArgumentTypeError(
            "Graph order must be between 2 and 12.")
    
    if not output_dir:
        test_support_dir = Path(sys.argv[0]).resolve().parent.parent
        output_parent_dir = Path.joinpath(
            test_support_dir, 'results', 'graph_generation_orchestrator'
        )
        candidate_output_dir = Path.joinpath(output_parent_dir, f"{order}")
        output_dir = candidate_output_dir
    elif not isinstance(output_dir, Path) or output_dir.is_file():
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

    Path.mkdir(output_dir, parents=True, exist_ok=True)

    return geng_path, order, output_dir


def distribute_geng_workload(
        geng_path: Path, canonical_files:bool, order: int,
        output_dir: Path):
    """Use starmap_async on multiprocessingn pool to _call_geng

    Args:
        geng_path: Path to the nauty geng executable
        order: Desired number of vertices
        generate_canonical: Bool to indicate whether or not to generate
            graphs with canonical labelling
        output_dir: Directory to which you wish to write the resulting .g6 file
    """
    geng_path, order, output_dir = _validate_and_normalize_geng_workload_args(
        geng_path, order, output_dir)

    Path.mkdir(output_dir, parents=True, exist_ok=True)

    call_geng_args = [
        (geng_path, canonical_files, order, edge_count, output_dir) 
        for edge_count in range(max_num_edges_for_order(order) + 1)
        ]

    with multiprocessing.Pool(processes=multiprocessing.cpu_count()) as pool:
        _ = pool.starmap_async(_call_geng, call_geng_args)
        pool.close()
        pool.join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        usage='python %(prog)s [options]',
        description="""Graph Generation Orchestrator

Orchestrates calls to nauty's geng to generate graphs for a given order,
separated out into files for each edge count. The output files will have paths:
    {output_dir}/{order}/n{order}.m{num_edges}.g6
""")
    parser.add_argument(
        '-g', '--gengpath',
        type=Path,
        metavar='PATH_TO_GENG_EXECUTABLE'
    )
    parser.add_argument(
        '-l', '--canonicalfiles',
        action='store_true'
    )
    parser.add_argument(
        '-n', '--order',
        type=int,
        default=11,
        metavar='N'
    )
    parser.add_argument(
        '-o', '--outputdir',
        type=Path,
        default=None,
        metavar='G6_OUTPUT_DIR',
        help="""If no output directory provided, defaults to
TestSupport/results/graph_generation_orchestrator/{order}"""
    )

    args = parser.parse_args()

    order = args.order
    canonical_files = args.canonicalfiles
    geng_path = args.gengpath
    output_dir = args.outputdir

    distribute_geng_workload(geng_path, canonical_files, order, output_dir)
