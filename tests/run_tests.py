import os
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

# run serial

subprocess.run(openmp_command)

for file in files:
    name = file[0] + '.' + file[1]
    with open(name + '.myout', 'w') as outfile:
        run_command = (['./life3d-omp', file[0] + '.in', file[1]]);
        print(BOLD + ' '.join(run_command) + RESET)
        start = time.time() 
        subprocess.run(run_command, stdout=outfile)
        end = time.time()
        diff_command = (['diff', name + '.out', name + '.myout'])
        result = subprocess.run(diff_command, stdout=subprocess.PIPE)
        print(RESET + "Time passed: %.4f" % (end - start) + "s")
        if result.stdout.decode() == '':
            print(GREEN + "Test successful" + RESET)
        else:
            print(RED + "Test failed" + RESET)

# run openmp

# run mpi

