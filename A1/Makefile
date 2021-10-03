build:
	gcc -fopenmp sb/sb.c util.c exporter.c goi_omp.c main.c -o goi_omp
	gcc sb/sb.c util.c exporter.c goi_threads.c main.c -o goi_threads -lpthread

clean:
	rm -f *.out *.gch
