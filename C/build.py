import os
import subprocess
import sys


def get_all_c_files(directory: str):
    print(f"Scanning Directory: {directory}")
    all_files = os.listdir(directory)
    c_files = []
    for file in all_files:
        file = f"{directory}\\{file}"
        if not os.path.isfile(file):
            c_files += [c_file for c_file in get_all_c_files(file)]
        elif len(file) > 2 and (file[-2:] == '.c' or file[-2:] == '.C'):
            c_files.append(file)
        else:
            pass # skip processing
    return c_files

def compile():
    current_directory = os.getcwd()
    all_files = get_all_c_files(current_directory)

    result = subprocess.run(["gcc", "-g"] + all_files, capture_output=True, text=True)

    exit_message = "Successful" if result.returncode == 0 else "Failure"
    print(f"\nCompilation {exit_message}\n")
    if result.stdout:
        print(f"Response: \n{result.stdout} \n")
    if result.stderr:
        print(f"Errors and warnings: \n{result.stderr} \n")

    if result.returncode == 1:
        raise Exception("Compilation Failed")


def run():
    current_directory = os.getcwd()
    result = subprocess.run([f"{current_directory}\\a.exe"], capture_output=True, text=True)

    exit_message = "Successful" if result.returncode == 0 else "Failure"
    print(f"Run {exit_message}\n")
    if result.stdout:
        print(f"Response: \n{result.stdout} \n")
    if result.stderr:
        print(f"Errors and warnings: \n{result.stderr} \n")

    if result.returncode == 1:
        raise Exception("Run Failed")

if __name__ == '__main__':
    commands = sys.argv[1:]

    if "compile" in commands:
        compile()
    if "run" in commands:
        run()

