gcc -o prog ordena_vetor.c -fopenmp
./prog 10

a openmpm do fractal:
gcc -o prog fractalomp.c -lm -fopenmp
./prog 100

as duas primeiras de mpi:
mpicc fractalmpiserial.c -o fractal -lm
mpirun -np 4 ./fractal 100

mpicc fractalmpi_io.c -o fractal -lm
mpirun -np 4 ./fractal 100