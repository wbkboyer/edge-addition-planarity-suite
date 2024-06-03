__all__ = [
    'TestAllGraphsOutputFileContentsError',
    'TestAllGraphsPathError',
    'process_file'
]

from pathlib import Path
import re

from planarity_constants import PLANARITY_ALGORITHM_SPECIFIERS

class TestAllGraphsPathError(BaseException):
    """
    Custom exception signalling issues with the paths of files that purportedly
    correspond to Test All Graphs output files.
    """

    def __init__(self, message):
        super().__init__(message)
        self.message = message


class TestAllGraphsOutputFileContentsError(BaseException):
    """
    Custom exception for representing errors that arise when processing
    files which purportedly contain the output produced by running the
    planarity Test All Graphs functionality for a single algorithm command
    """

    def __init__(self, message):
        super().__init__(message)
        self.message = message


def process_file(
        infile_path:Path, processed_data: dict={}, order: int=0,
        command: str=''
    )->tuple[dict, int, str]:
    """Process infile and integrate into processed_data dict

    Validates the infile name, then processes file contents and adds to the
    processed_data dict.

    Args:
        infile_path: Corresponds to a file within the input_dir
        processed_data: Dict to which we wish to add information parsed from
            the infile, namely the infilename, numGraphs, numOK,
            numNONEMBEDDABLE, errorFlag, and duration
        order: If other files have been processed previously, this value is
            used for cross-validation (i.e. new infile should contain results
            for graphs of the same order as was previously incorporated into
            the processed_data); defaults to 0.
        command: If other files have been processed previously, this value is
            used for cross-validation (i.e. new infile should contain results
            for running the same command on other .g6 files that were
            previously incorporated into the processed_data); defaults to the
            empty string.
    
    Returns:
        processed_data: Updated dict with contents of infile_path added
        order: 
        command: 

    Raises:
        TestAllGraphsPathError: If invalid infile_path
        TestAllGraphsOutputFileContentsError: If input file corresponds to
            results that have already been processed, or re-raises
            exception thrown by process_file_contents()
    """
    try:
        order, num_edges, command = \
            _validate_infile_name(infile_path, order, command)
    except TestAllGraphsPathError as e:
        raise TestAllGraphsPathError(
            "Unable to process file when given invalid infile name."
        ) from e
    else:
        if num_edges in processed_data.keys():
            raise TestAllGraphsOutputFileContentsError(
                "Already processed a file corresponding to "
                f"{num_edges} edges."
            )
        try:
            planarity_infile_name, duration, numGraphs, numOK, \
            numNONEMBEDDABLE, errorFlag \
                = _process_file_contents(infile_path, command)
        except TestAllGraphsOutputFileContentsError as e:
            raise TestAllGraphsOutputFileContentsError(
                f"Unable to process contents of '{infile_path}'."
            ) from e
        else:
            processed_data[num_edges] = {
                'infilename': planarity_infile_name,
                'numGraphs': numGraphs,
                'numOK': numOK,
                'numNONEMBEDDABLE': numNONEMBEDDABLE,
                'errorFlag': errorFlag,
                'duration': duration
            }

            return processed_data, order, command


def _validate_infile_name(
        infile_path:Path, order: int=0, command: str=''
    )->tuple[int, int, str]:
    """Checks that infile_path corresponds to output of running planarity
    
    Args:
        infile_path: pathlib.Path object indicating the input file whose
            name should be validated before processing

    Returns:
        order_from_filename, num_edges_from_filename, command_from_filename
    
    Raises:
        TestAllGraphsPathError: If infile_name doesn't match the expected
            pattern for an output file from planarity Test All Graphs,
            if the graph order indicated by the infile_name doesn't match
            previously processed files, if the num edges in the input graph
            doesn't make sense (greater than max_num_edges), if the
            algorithm command specifier isn't one of the supported values,
            or if the algorithm command specifier doesn't match previously 
            processed files.
    """
    infile_name = infile_path.parts[-1]
    match = re.match(
        r'n(?P<order>\d+)\.m(?P<num_edges>\d+)(?:\.canonical)?(?:\.g6)?\.' \
        r'(?P<command>[pdo234])\.out\.txt',
        infile_name)
    if not match:
        raise TestAllGraphsPathError(
            f"Infile name '{infile_name}' doesn't match pattern."
        )
    
    order_from_filename = int(match.group('order'))
    num_edges_from_filename = int(match.group('num_edges'))
    command_from_filename = match.group('command')

    if order and order != order_from_filename:
        raise TestAllGraphsPathError(
            f"Infile name '{infile_name}' indicates graph order doesn't"
            " equal previously derived order."
        )
    
    max_num_edges = ((order * (order - 1)) / 2)
    if num_edges_from_filename > max_num_edges:
        raise TestAllGraphsPathError(
            f"Infile name '{infile_name}' indicates graph num_edges is"
            " greater than possible for a simple graph."
        )

    if command_from_filename not in PLANARITY_ALGORITHM_SPECIFIERS():
        raise TestAllGraphsPathError(
            f"Infile name '{infile_name}' contains invalid algorithm "
            f"command, '{command_from_filename}'."
        )

    if command and command != command_from_filename:
        raise TestAllGraphsPathError(
            f"Command specified in input filename, '{command_from_filename}', "
            f"doesn't match previously derived algorithm command, '{command}'."
        )
    
    return order_from_filename, num_edges_from_filename, command_from_filename


def _process_file_contents(
        infile_path:Path, command: str
    )->tuple[str, float, int, int, int, str]:
    """Processes and validates input file contents

    Uses re.match() to determine whether the file contents are of the
    expected form and attempts to extract the values produced by running
    planarity Test All Graphs for a given algorithm command on a specific
    .g6 file containing all graphs of a given order and single edge-count.

    Args:
        infile_path: pathlib.Path object indicating the input file whose
            contents are validated and processed

    Returns:
        planarity_infile_name: extracted from infile_path.parts
        duration: How long it took to run the chosen graph algorithm on all
            graphs of the given order for the given number of edges
        numGraphs: total number of graphs processed in the .g6 infile
        numOK: number of graphs for which running the planarity algorithm
            specified by the command returned OK (i.e. gp_Embed() with
            embedFlags corresponding to the command returned OK and 
            gp_TestEmbedResultIntegrity() also returned OK)
        numNONEMBEDDABLE: number of graphs for which running the planarity 
            algorithm specified by the command returned NONEMBEDDABLE (i.e.
            gp_Embed() with embedFlags corresponding to the command
            returned NONEMBEDDABLE and gp_TestEmbedResultIntegrity() also 
            returned NONEMBEDDABLE) 
        errorFlag: either SUCCESS (if all graphs reported OK or
            NONEMBEDDABLE) or ERROR (if an error was encountered allocating
            memory for or managing the graph datastructures, if an error
            was raised by the G6ReadIterator, or if the Result from
            gp_Embed() doesn't concur with gp_TestEmbedResultIntegrity())

    Raises:
        TestAllGraphsOutputFileContentsError: If the input file's header
            doesn't have the expected format or values for those fields, if
            the body of the input file doesn't have the expected format, if
            the command derived doesn't match the expected algorithm
            command specifier, or if the error flag is something other than
            ERROR or SUCCESS.
    """
    with open(infile_path, 'r') as infile:
        line = infile.readline()
        match = re.match(
            r'FILENAME="(?P<filename>n\d+\.m\d+(\.canonical)?\.g6)"' \
            r' DURATION="(?P<duration>\d+\.\d{3})"', line)
        if not match:
            raise TestAllGraphsOutputFileContentsError(
                "Invalid file header."
            )
        
        planarity_infile_name_from_file = match.group('filename')
        if not planarity_infile_name_from_file:
            raise TestAllGraphsOutputFileContentsError(
                "Header doesn't contain input filename."
            )
        
        duration_from_file = match.group('duration')
        if not duration_from_file:
            raise TestAllGraphsOutputFileContentsError(
                "Unable to extract duration from input file."
            )
        
        duration_from_file = float(duration_from_file)

        line = infile.readline()
        match = re.match(
            r'-(?P<command>\w) (?P<numGraphs>\d+) ' \
            r'(?P<numOK>\d+) (?P<numNONEMBEDDABLE>\d+) ' \
            r'(?P<errorFlag>SUCCESS|ERROR)', line
        )
        if not match:
            raise TestAllGraphsOutputFileContentsError(
                'Invalid file contents.'
            )
        
        command_from_file = match.group('command')
        if command and command != command_from_file:
            raise TestAllGraphsOutputFileContentsError(
                f"Command specified in input file, '{command_from_file}', "
                "doesn't match command "
                f"given in input filename, '{command}'."
            )
        
        if command_from_file not in PLANARITY_ALGORITHM_SPECIFIERS():
            raise TestAllGraphsOutputFileContentsError(
                f"Command specifier '{command_from_file}' in input file "
                f"'{infile_path}' is not valid"
            )
        
        numGraphs_from_file = match.group('numGraphs')
        numOK_from_file = match.group('numOK')
        numNONEMBEDDABLE_from_file = match.group('numNONEMBEDDABLE')

        errorFlag_from_file = match.group('errorFlag')
        if (
            not errorFlag_from_file or 
            errorFlag_from_file not in ('SUCCESS', 'ERROR')
        ):
            raise TestAllGraphsOutputFileContentsError(
                "Invalid errorFlag; must be SUCCESS or ERROR"
            )
        
        return planarity_infile_name_from_file, duration_from_file, \
            numGraphs_from_file, numOK_from_file, \
            numNONEMBEDDABLE_from_file, errorFlag_from_file
