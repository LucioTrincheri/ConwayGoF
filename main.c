#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include "game/Board.h"
#include "game/Game.h"


/*
Tenemos estado viejo y nuevo (arrays bidimencionales (?), el viejo solo se lee, no se modifica por lo tanto no es necesario lockear.
El nuevo se edita, pero si hacemos que cada thread se ocupe de una zona no hace falta lockear.
Necesitamos semaforos para que no se ejecuten mas thread simultaneos que cores disponibles (get_nprocs())
Y barreras para que todos ejecuten su actualizacion y luego se muestre la generacion.
*/


// Crear una instancia de game, 
// Game: leer el archivo
// Main: crear los pthreads, hacer funcion que agarrando los tableros hacen una actualizacion (barreras para esperar).
// En tablero nuevo va aestar la nueva generacion. Pasarla a tablero viejo y crear un nuevo tablero
// vacio. cont ++. Repetir. Al finalizar, llamar:
// Game: writeBoard y gg

typedef struct _argshilo{
	int inicio;
	int final;
	board_t *viejo;
	board_t *nuevo;
	int ciclos;
} argshilo;

pthread_barrier_t barrier;

void *trabajo_thread(argshilo *arg){
	board_t *nuevoThread = arg->nuevo;
	board_t *viejoThread = arg->viejo;
	board_t *aux;
	
	for (int i = 0; i < arg->ciclos; i++) {
		// intervambiamos viejo con nuevo y llamamos a nueva generacion, cuando 
		// todos los threads lo hagan seguimos con el ciclo.
		
		aux = nuevoThread;
		nuevoThread = viejoThread;
		viejoThread = aux;
	
		nueva_generacion_tablero(arg->inicio, arg->final,  viejoThread,  nuevoThread);
		pthread_barrier_wait(&barrier);
		
	}

	free(arg);
	
	pthread_exit(0);
}

int main()
{
	
	game_t *game = loadGame("tablero.txt");
	board_t *viejo = board_init(game->board->columnas, game->board->filas);
	board_t *nuevo = game->board;

	
	//int nthread = 2;
	int nthread = get_nprocs();
	int *interv;
	interv = interv_filas_pthr(nuevo, nthread);
	pthread_t threads[nthread];
	pthread_barrier_init(&barrier, NULL, nthread);
	
	for (int i = 0; i < nthread; i++) {
		argshilo *arg = malloc(sizeof (argshilo));
		arg->inicio = interv[2*i];
		arg->final = interv[(2*i) + 1];
		arg->viejo = viejo;
		arg->nuevo = nuevo;
		arg->ciclos = game->ciclos;
		pthread_create(&threads[i], NULL, (void *)trabajo_thread, (void *) arg);
	}

	
	for (int i = 0;i < nthread; i++) 
		pthread_join(threads[i], NULL);
	
	pthread_barrier_destroy(&barrier);

	
	/*
			for(unsigned int i = 0; i < nuevo->filas; i++){
			for(unsigned int j = 0; j < nuevo->columnas; j++){
				printf("%c", nuevo->grilla[i][j]);
			}
			printf("\n");
		}
	*/
	
	
	// Escritura del estado final
	if (game->ciclos % 2 == 0){
		writeBoard(nuevo, "resultado.txt");
	}
	else {
		writeBoard(viejo, "resultado.txt");
	}
	
	
	free(interv);

	board_destroy(viejo);
	game_destroy(game);

	
	return 0;
}
