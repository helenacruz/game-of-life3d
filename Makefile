life3d-mpi: life3d-mpi.cpp 
	mpic++ -g -o life3d-mpi life3d-mpi.cpp

clean:
	rm -f life3d-mpi

run: 
	mpirun -np $(n) life3d-mpi