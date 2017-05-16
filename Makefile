life3d-mpi: life3d-mpi.cpp 
	mpic++ -std=c++11 -fopenmp -o life3d-mpi life3d-mpi.cpp

clean:
	rm -f life3d-mpi

run: 
	mpirun -np $(n) life3d-mpi $(f) $(gen)
