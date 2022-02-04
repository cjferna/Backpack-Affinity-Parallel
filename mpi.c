/*
CPP_CONTEST=2013Cal-2011
CPP_PROBLEM=R
CPP_LANG=C+OPENMP+MPI
CPP_PROCESSES_PER_NODE=marte 6 mercurio 6
*/

//

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#define NIVELES_GENERAR 4 	// Niveles del arbol generados.

struct Nodo {
	int *solucion;			// solucion[x]. Cada Posicion indica a que mochila va el objeto x.
	int *pesoPorMochila;	// pesoPorMochila[x]. Peso de cada mochila actual. Peso de la mochila x.
	int nivel;				// Nivel acutal. Indica el objeto actual.
};

// Genera un nodo
void generar(struct Nodo *nodo, int *pesosObjetos) {
	// Si antes había un objeto en esta posición, se resta su peso.
	if (nodo->solucion[ nodo->nivel ] != -1) {
		nodo->pesoPorMochila[ nodo->solucion[ nodo->nivel ] ] -= pesosObjetos[ nodo->nivel ];
	}
	
	// Se salta al siguiente objeto. Y se acumula el peso de dicho objeto.
	nodo->solucion[ nodo->nivel ]++;
	nodo->pesoPorMochila[ nodo->solucion[ nodo->nivel ] ] += pesosObjetos[ nodo->nivel ];
}

// Determina si "solucion" comprende una solucion del problema.
int esSolucion(struct Nodo *nodo, int numeroObjetos, int *capacidadesMochilas) {
	// Si se ha llegado al último nivel, siendo por tanto, nivel == último nivel == ultimo objeto. 
	// Y si el peso de la mochila que contiene el último objeto (último nivel) no supera la capacidad de la mochila asginada al objeto (solucion[nivel]).	
	return ((nodo->nivel == numeroObjetos - 1) 
		   && nodo->pesoPorMochila[ nodo->solucion[ nodo->nivel ] ] <= capacidadesMochilas[ nodo->solucion[ nodo->nivel ] ]);
}

// Calcula el valor de "solucion"
int calcularValor(int numeroObjetos, struct Nodo *nodo, int *afinidad) {
	int i, j;
	int valor = 0;	// Resultado, valor de la solución. Beneficio final.

	// Para cada objeto comparado con el resto de objetos restantes.
	for (i = 0; i < numeroObjetos; i++) {
		for (j = i + 1; j < numeroObjetos; j++) {
			// Si ambos objetos están en la misma mochila. solucion[x] --> Almacena la mochila donde cae el objeto x.
			if (nodo->solucion[i] == nodo->solucion[j]) {
				// Se acumula el valor de la afinidad de ambos objetos.
				valor += (afinidad[i*numeroObjetos + j] + afinidad[j*numeroObjetos + i]);
			}
		}
	}
	
	return valor;
}

// Determina si se puede continuar hacia le siguiente nivel.
int criterio(struct Nodo *nodo, int numeroObjetos, int *capacidadesMochilas) {
	// Si el nivel es distinto del último nivel.
	// Y si el peso de la mochila que contiene el último objeto añadido (nivel) no supera la capacidad de la mochila asginada al objeto (solucion[nivel]).	
	return (nodo->nivel != numeroObjetos - 1 && nodo->pesoPorMochila[ nodo->solucion[ nodo->nivel ] ] <= capacidadesMochilas[ nodo->solucion[ nodo->nivel ] ]);
}

// Determina si quedan hermanos en el mismo nivel.
int hayMasHermanos(struct Nodo *nodo, int numeroMochilas) {
	// Si aún quedan mochilas en las que probar introducir el objeto actual (nivel).
	return (nodo->solucion[ nodo->nivel ] < numeroMochilas - 1);
}

// Elimina el objeto actual, por tanto, retroce en nivel.
void retroceder(struct Nodo *nodo, int numeroMochilas, int *pesosObjetos) {
	// Se elimina el peso del último objeto añadido. 
	// Se ha modificado numeroMochilas - 1 por solucion[nivel]. Ya que el peso se le resta a la mochila donde está el último objeto.
	nodo->pesoPorMochila[ numeroMochilas - 1 ] -= pesosObjetos[ nodo->nivel ];
	// Se marca como que dicho objeto(nivel) no está presente en ninguna mochila.
	nodo->solucion[ nodo->nivel ] = -1;
	// Se disminuye el nivel.
	nodo->nivel--;
}

// Imprime un nodo.
void escribir(struct Nodo *nodo, int numeroMochilas, int numeroObjetos) {
	int i;

	printf("En nivel %d\n", nodo->nivel);
	printf("	solucion:");
	for(i = 0; i < numeroObjetos; i++)
		printf(" %d ",nodo->solucion[i]);
	printf("\n");
	
	printf("	Pesos por mochila:");
	for(i = 0; i < numeroMochilas; i++)
		printf(" %d ", nodo->pesoPorMochila[i]);
	printf("\n");
	
	printf("\n");
}

// Eleva un número a su exponente.
int eleverA(int base, int exponente) {
	int resultado = 1;	
	
	int i;
	for (i = 0; i < exponente; i++)
		resultado *= base;
	
	return resultado;
}

// Inicializa una solución. Inicialmente ningún objeto está asignado a una mochila, y toda mochila tiene peso cero.
int inicializarEspacioSolucion(int numeroMochilas, int numeroObjetos, struct Nodo *nodo) {	
	int i;	
	
	// Ningún objeto está asignado a una mochila.
	for (i = 0; i < numeroObjetos; i++)
		nodo->solucion[i] = -1;

	// Todas las mochilas estan vacías.
	for (i = 0; i < numeroMochilas; i++)
		nodo->pesoPorMochila[i] = 0; 
	
	// Se comienza por el nivel cero.
	nodo->nivel = 0;
}

// Reserva memoria para almacenar una solución. Lista de mochilas donde se guardan los objetos y lista de pesos de cada mochila.
int crearEspacioSolucion(int numeroMochilas, int numeroObjetos, struct Nodo **nodo) {
	// Se reserva memoria de la estructura.
	*nodo = (struct Nodo*) malloc(sizeof(struct Nodo));
	
	// Se comienza por el nivel cero.
	(*nodo)->nivel = 0;

	// Todas las mochilas estan vacías.
	(*nodo)->pesoPorMochila = (int*) calloc (numeroMochilas, sizeof(int));
	
	// Ningún objeto está asignado a una mochila.
	(*nodo)->solucion = (int*) malloc (sizeof(int) * numeroObjetos);
	int i;
	for (i = 0; i < numeroObjetos; i++)
		(*nodo)->solucion[i] = -1;
}

// Destruye la memoria de una solución. 
int destruirEspacioSolucion(struct Nodo *nodo) {		
	free(nodo->solucion);
	free(nodo->pesoPorMochila);
	free(nodo);
}

struct Nodo** crearEspacioTareas(int numeroMochilas) {
	// El máximo de tareas es mochilas^niveles;
	int tamanoMaximoLista = eleverA(numeroMochilas, NIVELES_GENERAR);
	// Punto a la lista de tareas.
	return (struct Nodo**) malloc(sizeof(struct Nodo*) * tamanoMaximoLista);		
}

// Libera la memoria de la lista de tareas.
void liberarEspacioTareas(int tareasGeneradas, struct Nodo **tareas) {
	int i;
	
	// Se libera la memoria de los nodos de la lista de tareas.
	for (i = 0; i < tareasGeneradas; i++) {
		destruirEspacioSolucion(tareas[i]);	
	}
	
	free (tareas);	// Se libera la lista de tareas.	
}

// Realiza una copia en nodoSalida.
int copiarNodo(struct Nodo *nodoOrigen, struct Nodo *nodoDestino, int numeroObjetos, int numeroMochilas) {		
	// Se copia la solución actual.
	memcpy(nodoDestino->solucion, nodoOrigen->solucion, sizeof(int) * numeroObjetos);
	// Se copian los pesos de las mochilas.
	memcpy(nodoDestino->pesoPorMochila, nodoOrigen->pesoPorMochila, sizeof(int) * numeroMochilas);

	// Se copia el nivel.
	nodoDestino->nivel = nodoOrigen->nivel;
}

// Inicializa un nodo metiendo el primer objeto en la mochila especificada.
void nodoInicial(struct Nodo * nodo, int nivel, int mochila, int *pesosObjetos) {
	// Se especifica el nivel del nodo.
	nodo->nivel = nivel;	
	
	// Si antes había un objeto en esta posición, se resta su peso.
	if (nodo->solucion[ nodo->nivel ] != -1) {
		nodo->pesoPorMochila[ nodo->solucion[ nodo->nivel ] ] -= pesosObjetos[ nodo->nivel ];
	}

	// Se guarda el objeto nivel, en la mochila especificada.
	nodo->solucion[ nodo->nivel ] = mochila;
	// Se añade el peso del objeto.
	nodo->pesoPorMochila[ nodo->solucion[ nodo->nivel ] ] += pesosObjetos[ nodo->nivel ];
	// Se aumenta el nivel del nodo.
	nodo->nivel++;
}

// Crea las tareas correspondientes a un nivel. Si es nivel objetivo se almacenan en la lista, en otro caso se generan los hijos.
void crearTareas(struct Nodo * raiz, struct Nodo** tareas, int nivelActual, 
				 int nivelesGenerar, int numeroMochilas, int numeroObjetos, int *pesosObjetos, int *capacidadesMochilas, int *tareasGeneradas) {
	// Si no hay que generar niveles se vuelve.
	if (nivelesGenerar == 0) {
		return;
	}
	// Si el numero de niveles es mayor o igual que el numero de objetos, se generan tantos como objetos menos 1.
	else if (nivelesGenerar >= numeroObjetos) {
		nivelesGenerar = numeroObjetos - 1;
	}
	
	// Variables para la generacion de tareas.
	struct Nodo *nodoActual, *nodoAnterior;		// NodoActual, y NodoAnterior Generado
	crearEspacioSolucion(numeroMochilas, numeroObjetos, &nodoActual);	
	copiarNodo(raiz, nodoActual, numeroObjetos, numeroMochilas);	// Se copia el nodo raiz, parámetro, al nodoActual.
	
	int m;	
	// Para cada mochila.
	for (m = 0; m < numeroMochilas; m++) {
		// Se genera un nivel en el nodo actual.
		generar(nodoActual, pesosObjetos); 
		
		// Si el nodo cumple el criterio se almacena como tarea y se pasa a generar el siguiente nodo.
		if (criterio(nodoActual, numeroObjetos, capacidadesMochilas)) {			
			// Se crea el nuevo nodo como copia del anterior.
			nodoAnterior = nodoActual;
			crearEspacioSolucion(numeroMochilas, numeroObjetos, &nodoActual);
			copiarNodo(nodoAnterior, nodoActual, numeroObjetos, numeroMochilas);
			
			// Se aumenta de nive1 al nodo Anterior.
			nodoAnterior->nivel++;		
			
			// Si el nivel generado no es el nivel objeto, se generan sus hijos.
			if (nivelActual + 1 < nivelesGenerar) {
				crearTareas(nodoAnterior, tareas, nivelActual+1, nivelesGenerar, numeroMochilas, numeroObjetos, pesosObjetos, capacidadesMochilas, tareasGeneradas);
			// En otro caso, se almacena este nodo que está en el nivel objetivo.
			} else {			
				// Se almacena como tarea. Y se avanza en la tarea actual.
				tareas[*tareasGeneradas] = nodoAnterior;					
				(*tareasGeneradas)++;
			}
		}
	}
}

// Calcula los indices de inicio y fin de cada proceso en el array 'inicioTrozos' de los nodos generados que le corresponde procesar.
void calcularRepartoNodosEntreProcesos(int numeroNodosNivel, int *inicioTrozos, int np) {
	int inicio = 0;				// Indica el inicio del trozo que le corresponde.
	int trozos[np];				// Tamano de trozo por proceso.
	
	int i;
	// Para cada cada proceso. Se almacena el inicio de cada trozo, y su fin.
	for (i = 0; i < np; i++) {
		trozos[i] = numeroNodosNivel / np; 	// Es el tamano por proceso.		
		// Si sobran nodos.
		if (i < numeroNodosNivel % np)
			trozos[i]++;		 	// Lo que sobra se le suma uno a cada uno desde el principio.
		
		inicioTrozos[i] = inicio;   // El inicio de este trozo es el final del anterior.
		inicio += trozos[i];		// Se suma el tamano del trozo actual al inicio.
	} 
	
	inicioTrozos[np] = inicio; 		// El final del último proceso.	
}

// Genera las tareas en paralelo. 
void generarTareas(int numeroMochilas, int numeroObjetos, int *capacidadesMochilas, int *pesosObjetos, int *tareasGeneradas, struct Nodo **tareas, int mpiNode, int np) {
	int numeroNiveles = 3;
	
	// Lista de nodos inicial, a partir de la cual se generará el reparto.
	int numeroNodosNivel = 0;
	struct Nodo **nodosSegundoNivel = (struct Nodo**) malloc(sizeof(struct Nodo*) * eleverA(numeroMochilas, numeroNiveles));
	
	/** Se crean los nodos hasta el segundo nivel.*/
	struct Nodo *nodo;
	crearEspacioSolucion(numeroMochilas, numeroObjetos, &nodo);		// Se crea un nodo raíz.
	crearTareas(nodo, nodosSegundoNivel, 0, numeroNiveles, numeroMochilas, numeroObjetos, pesosObjetos, capacidadesMochilas, &numeroNodosNivel);
	
	/** Se calcula el reparto de los nodos entre los procesos */
	int inicioTrozos[np + 1];	// Inicio del trozo para cada proceso.
	calcularRepartoNodosEntreProcesos(numeroNodosNivel, inicioTrozos, np);
	
	/** Se desarrollan los hijos del trozo que le corresponde al proceso. */
	int i;
	for (i = inicioTrozos[mpiNode]; i < inicioTrozos[ mpiNode + 1 ]; i++) {	
		crearTareas(nodosSegundoNivel[i], tareas, numeroNiveles + 1, NIVELES_GENERAR, numeroMochilas, numeroObjetos, pesosObjetos, capacidadesMochilas, tareasGeneradas);
	}
	
	// Se borra el nodo generado.
	destruirEspacioSolucion(nodo);
	// Se libera los nodos generados hasta el segundo nivel.
	liberarEspacioTareas(numeroNodosNivel, nodosSegundoNivel);
}

// Método secuencial de backtracking para un nodo.
void backtracking(struct Nodo *nodo, int *VOAPersonal, int numeroMochilas, int numeroObjetos, int *capacidadesMochilas, int *pesosObjetos, int *afinidad) {
	int nivelInicial = nodo->nivel;
	
	do {
		generar(nodo, pesosObjetos); 
	
		// Si es solución, se calcula su valor y se almacena el mayor.
		if (esSolucion(nodo, numeroObjetos, capacidadesMochilas)) {
			int valor = calcularValor(numeroObjetos, nodo, afinidad);
			if (valor > *VOAPersonal) {
				*VOAPersonal = valor;
			}
		}
		
		// Si cumple el criterio para subir de nivel.
		if (criterio(nodo, numeroObjetos, capacidadesMochilas)) {
			nodo->nivel++;
		}
		// En otro caso, se retrocede mientras se pueda.
		else {
			while (nodo->nivel >= nivelInicial && !hayMasHermanos(nodo, numeroMochilas)) {
				retroceder(nodo, numeroMochilas, pesosObjetos);
			}
		}
		
	} while (nodo->nivel >= nivelInicial);
}

// Procesa las tareas de la lista en paralelo. 
void procesarTareas(int numeroMochilas, int numeroObjetos, int *capacidadesMochilas, int *pesosObjetos, int *afinidad,
					int *tareaActual, int *VOAPersonal, int tareasGeneradas, struct Nodo **tareas) {
	int i;
	for (i = 0; i < tareasGeneradas; i++) {
		// Se extrae el nodo de la lista de tareas, y se aplica el secuencial al nodo.	
		backtracking(tareas[i], VOAPersonal, numeroMochilas, numeroObjetos, capacidadesMochilas, pesosObjetos, afinidad);
	}
}

// El nodo 0 recibo los valores VOA y establece el mayor valor a devolver.
int establecerMayorValorOptimoActualDistribuido(int VOAPersonal, int *resultado, int mpiNode, int np) {	
	// Si no es el nodo cero, envía su VOA encontrado.
	if (mpiNode != 0) {
		MPI_Send(&VOAPersonal, 1, MPI_INT, 0, 10, MPI_COMM_WORLD);
	}
	// Si es el nodo cero, recoge los VOA de los proceso y almacena el mayor.
	else { 
		// Se inicializa con el valor del proceso 0.
		(*resultado) = VOAPersonal;
		int resultadoThread;
		
		int p;
		// Por cada proceso, se recoge su valor.
		for (p = 1; p < np; p++) {
			// Se recibe el VOA del proceso 'p'.
			MPI_Recv(&resultadoThread, 1, MPI_INT, p, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			// Si su VOA es mayor. Se actualiza.			
			if (resultadoThread > (*resultado)) {
				 (*resultado) = resultadoThread;
			}
		}
	}
}

void repartirDatos(int *numeroMochilas, int *numeroObjetos, int **capacidadesMochilas, int **pesosObjetos, int **afinidad, int *mpiNode, int *np) {
	// Datos propios del proceso.
	MPI_Comm_size(MPI_COMM_WORLD, np);
	MPI_Comm_rank(MPI_COMM_WORLD, mpiNode);
	
	MPI_Bcast(numeroMochilas, 1, MPI_INT, 0, MPI_COMM_WORLD); 	// Número de mochilas.
	MPI_Bcast(numeroObjetos, 1, MPI_INT, 0, MPI_COMM_WORLD);	// Número de Objetos.
	
	// Si no es el nodo cero, reservan memoria.
	if ((*mpiNode) != 0) {
		(*capacidadesMochilas) = (int *) malloc(sizeof(int) * (*numeroObjetos));
		(*pesosObjetos) = (int *) malloc(sizeof(int) * (*numeroObjetos));
		(*afinidad) = (int *) malloc(sizeof(int) * (*numeroObjetos) * (*numeroObjetos));
	}
	
	MPI_Bcast(*capacidadesMochilas, (*numeroMochilas), MPI_INT, 0, MPI_COMM_WORLD); 	// Capacidades de las mochilas
	MPI_Bcast(*pesosObjetos, (*numeroObjetos), MPI_INT, 0, MPI_COMM_WORLD);				// Peso de cada objeto..
	MPI_Bcast(*afinidad, (*numeroObjetos) * (*numeroObjetos), MPI_INT, 0, MPI_COMM_WORLD);	// Afinidad, beneficio por par de objetos.
}

void liberarEspacioDatos(int mpiNode, int *capacidadesMochilas, int *pesosObjetos, int *afinidad) {
	// Si no es el nodo cero, liberan memoria.
	if (mpiNode != 0) {
		free(capacidadesMochilas);
		free(pesosObjetos);
		free(afinidad);
	}
}


int sec(int numeroMochilas, int numeroObjetos, int *capacidadesMochilas, int *pesosObjetos, int *afinidad, int mpiNode, int np) {
	
	repartirDatos(&numeroMochilas, &numeroObjetos, &capacidadesMochilas, &pesosObjetos, &afinidad, &mpiNode, &np);
	struct Nodo **tareas = crearEspacioTareas(numeroMochilas);	// Lista de tareas.
	
	int tareasGeneradas = 0; // Numero total de tareas generadas.
	int tareaActual = 0;	 // Variable compartida, primera tarea no asignada.
	int resultado = 0;		 // Valor a devolver. Beneficio optimo.
	
	int VOAPersonal = 0;	// Valor Optimo Actual de cada thread.			
	generarTareas(numeroMochilas, numeroObjetos, capacidadesMochilas, pesosObjetos, &tareasGeneradas, tareas, mpiNode, np);
	procesarTareas(numeroMochilas, numeroObjetos, capacidadesMochilas, pesosObjetos, afinidad, &tareaActual, &VOAPersonal, tareasGeneradas, tareas);
	establecerMayorValorOptimoActualDistribuido(VOAPersonal, &resultado, mpiNode, np);
	
	liberarEspacioTareas(tareasGeneradas, tareas);		
	liberarEspacioDatos(mpiNode, capacidadesMochilas, pesosObjetos, afinidad);
	
	return resultado;
}