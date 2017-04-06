import os
import sys
import subprocess
import time

# colors

RED   = "\033[1;31m"  
GREEN = "\033[0;32m"
RESET = "\033[0;0m"
BOLD    = "\033[;1m"

# commands

serial_command = (['g++', '../life3d.cpp', '-o', 'life3d', '-std=c++11'])
openmp_command = (['g++', '-fopenmp', '../life3d-omp.cpp', '-o', 'life3d-omp', '-std=c++11'])

# get files

dir_files = []
for file in os.listdir('.'):
    if file.endswith('.out'):
       dir_files += [file]

files = []

for file in dir_files:
    files += [str.split(file, '.')]

def run_tests(exec_name):
    for file in files:
        name = file[0] + '.' + file[1]
        with open(name + '.myout', 'w') as outfile:
            run_command = ([exec_name, file[0] + '.in', file[1]]);
            print(BOLD + ' '.join(run_command) + RESET)
            start = time.time() 
            subprocess.run(run_command, stdout=outfile)
            end = time.time()
            diff_command = (['diff', name + '.out', name + '.myout'])
            result = subprocess.run(diff_command, stdout=subprocess.PIPE)
            print(RESET + "Time passed: %.4f" % (end - start) + "s")
            print(result.stdout.decode())
	    #if result.stdout.decode() == '':
            #    print(GREEN + "Test successful" + RESET + "\n")
            #else:
            #    print(RED + "Test failed" + RESET + "\n")

if len(sys.argv) != 2:
    print("Usage:")
    print("To run serial version: python3 run_tests.py -serial")
    print("To run OpenMP version: python3 run_tests.py -openmp")
    print("To run MPI version: python3 run_tests.py -mpi")

elif sys.argv[1] == '-serial':
    print("Running serial version\n")
    subprocess.run(serial_command)
    run_tests('./life3d')
    print("Done\n")

elif sys.argv[1] == '-openmp':
    print("Running OpenMP version\n")
    subprocess.run(openmp_command)
    run_tests('./life3d-omp')
    print("Done\n")

elif sys.argv[1] == '-mpi':
    print("Running MPI version\n")
    subprocess.run(mpi_command)
    run_tests('./life3d-mpi')
    print("Done\n")

else:
    print("Option not available, try -serial, -openmp or -mpi")

