build_normal:
	gcc sb/sb.c util.c exporter.c goi.c main.c -o goi.out

build_omp:
	gcc -fopenmp sb/sb.c util.c exporter.c goi_omp.c main.c -o goi_omp.out

build_threads:
	gcc sb/sb.c util.c exporter.c goi_thread_n.c main.c -o goi_threads.out -lpthread
clean:
	rm -f *.out *.gch
