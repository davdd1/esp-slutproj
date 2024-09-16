import os
import argparse

def main():
    parser = argparse.ArgumentParser(description='Concatenate all programming files into a single text file.')
    parser.add_argument('-d', '--directory', default='.', help='Root directory to scan')
    parser.add_argument('-o', '--output', default='all_programming_files.txt', help='Output file name')
    parser.add_argument('-e', '--extensions', nargs='+', help='List of file extensions to include', default=[
        '.py', '.java', '.c', '.cpp', '.h', '.js', '.rb', '.php', '.go', '.cs', '.swift', '.kt',
        '.html', '.css', '.ts', '.sql', '.json', '.xml', '.sh', '.pl', '.r', '.m', '.erl', '.ex',
        '.exs', '.scala', '.lhs', '.hs', '.ml', '.fs', '.fsx'
    ])
    parser.add_argument('-x', '--exclude', nargs='+', help='List of file extensions to exclude', default=[])

    args = parser.parse_args()

    programming_extensions = args.extensions
    exclude_extensions = args.exclude
    output_file = args.output
    root_dir = args.directory

    # Define directories to ignore
    ignore_dirs = {'.vscode', '.devcontainer', 'build', 'managed_components'}

    with open(output_file, 'w', encoding='utf-8') as outfile:
        for dirpath, dirnames, filenames in os.walk(root_dir):
            # Modify dirnames in-place to skip ignored directories
            dirnames[:] = [d for d in dirnames if d not in ignore_dirs]

            for filename in filenames:
                # Check if file has an allowed extension and is not in the exclude list
                if (any(filename.endswith(ext) for ext in programming_extensions) and
                    not any(filename.endswith(ext) for ext in exclude_extensions)):
                    file_path = os.path.join(dirpath, filename)
                    try:
                        with open(file_path, 'r', encoding='utf-8') as infile:
                            content = infile.read()
                        outfile.write(f"\n\n{'='*80}\n")
                        outfile.write(f"File: {file_path}\n")
                        outfile.write(f"{'='*80}\n\n")
                        outfile.write(content)
                    except Exception as e:
                        print(f"Error reading {file_path}: {e}")

if __name__ == '__main__':
    main()
