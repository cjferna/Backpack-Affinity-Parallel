/*
CPP_CONTEST=2013Cal-2011
CPP_PROBLEM=Q
CPP_LANG=C+OPENMP
CPP_PROCESSES_PER_NODE=jupiter 1
*/

/* RECORD
Carlos Javier Fernández Candel
student of Methodology of Parallel Programming
at the University of Murcia
January 14, 2016
time 753 msec
speedup 31.08*/

/*
 Tasks list algorithm scheme is used.
 Nodes are generated and saved on a list in parallel until level specified('LEVELS_TO_GENERATE'). That node list is 
 computed in parallel, where every thread takes as nodes as ('NUM_RESERVED_NODES'), to prevent critical waste time. 
 Finally, all threads tries to put his personal best result 'VOA' as global better result.
*/

#include <stdlib.h>
#include <omp.h>
#include <string.h>

#define NUM_RESERVED_NODES 5 	// Numbres of nodes to reserve to prevent critical wait time.	
#define LEVELS_TO_GENERATE 4 	// Numbers of levels to generate at start to balance compute distribution.

// Represent a node of solution.
struct Node {
	int *solution;			// solution[x]. Every posistion save the backpack where obejct 'x' is saved.
	int *backpacksWeights;	// backpacksWeights[x]. Weight of every backpack. Weight of backpack 'x'.
	int level;				// Actual level. Actual ojbect.
};

// Envolve a node. Put the actual object (level) on the next backpack.
void generate(struct Node *node, int *objectsWeights) {
	// If a object where in this position, his weight is subtracted.
	if (node->solution[ node->level ] != -1) {
		node->backpacksWeights[ node->solution[ node->level ] ] -= objectsWeights[ node->level ];
	}
	
	// Put the actual object (level) on the next backpack. His weight is added to the backpack.
	node->solution[ node->level ]++;
	node->backpacksWeights[ node->solution[ node->level ] ] += objectsWeights[ node->level ];
}

// Determine if node is a complete solution.
int isSolution(struct Node *node, int numbersOfObjects, int *backpacksCapacities) {
	// If it's last level. level == last level == last object.
	// And if the weight of the backpack where te actual object is saved (last level) is not over the backpack capacity. (Backpack where object is saved :solution[ level ] ]) 
	return ((node->level == numbersOfObjects - 1) 
		   && node->backpacksWeights[ node->solution[ node->level ] ] <= backpacksCapacities[ node->solution[ node->level ] ]);
}

// Value of this solution is calculated.
int calculateValue(int numbersOfObjects, struct Node *node, int *affinity) {
	int i, j;
	int value = 0;	// Result, value of this solution. Benefit.

	// For every object compared with the rest of objects.
	for (i = 0; i < numbersOfObjects; i++) {
		for (j = i + 1; j < numbersOfObjects; j++) {
			// If both object are in the same packack. solution[x] indicates backpack where object 'x' is saved.
			if (node->solution[i] == node->solution[j]) {
				// Value is added as affinity of both objects.
				value += (affinity[i*numbersOfObjects + j] + affinity[j*numbersOfObjects + i]);
			}
		}
	}
	
	return value;
}

// Determine  if a node can grown to the next level. 
int criteria(struct Node *node, int numbersOfObjects, int *backpacksCapacities) {
	// If the levels is not the last level.
	// And if the weight of the backpack where last object where saved is not over the capacity of the backpack.
	return (node->level != numbersOfObjects - 1 && node->backpacksWeights[ node->solution[ node->level ] ] <= backpacksCapacities[ node->solution[ node->level ] ]);
}

// Determine if this level has more brothers. 
int hasMoreBrothers(struct Node *node, int numbersOfBackpacks) {
	// If all backpacks are not used to try to save the current object (level).
	return (node->solution[ node->level ] < numbersOfBackpacks - 1);
}

// Deletes the actual object. Node level will be reduced.
void goBack(struct Node *node, int numbersOfBackpacks, int *objectsWeights) {
	// The weight of the last object(level) is subtracted.
	node->backpacksWeights[ numbersOfBackpacks - 1 ] -= objectsWeights[ node->level ];
	// The actual object is marked as not saved in a backpack.
	node->solution[ node->level ] = -1;
	// Level is reduced.
	node->level--;
}

// Base number to an exponent. To not link math.h -lm.
int raiseTo(int base, int exponente) {
	int result = 1;	
	
	int i;
	// Base is multiplied by base, 'exponente' times.
	for (i = 0; i < exponente; i++)
		result *= base;
	
	return result;
}

// Inicializate a solution. No one object is saved to a backpack. All backpacks weigth is zero.
int initializeSolutionSpace(int numbersOfBackpacks, int numbersOfObjects, struct Node *node) {	
	int i;	
	
	// No one object is saved to a backpack.
	for (i = 0; i < numbersOfObjects; i++)
		node->solution[i] = -1;

	// All backpacks weigth is zero.
	for (i = 0; i < numbersOfBackpacks; i++)
		node->backpacksWeights[i] = 0; 
	
	// The start level is zero.
	node->level = 0;
}

// Reserva memoria para almacenar una solución. Lista de mochilas donde se guardan los objetos y lista de pesos de cada mochila.
// Memory is allocated to save an solution(node). Objets assing array and Backpacks weights array.
int createSolutionSpace(int numbersOfBackpacks, int numbersOfObjects, struct Node **node) {
	// Struct memory allocated.
	*node = (struct Node*) malloc(sizeof(struct Node));
	
	// Starting level is zero.
	(*node)->level = 0;

	// All backpacks weigth is zero.
	(*node)->backpacksWeights = (int*) calloc (numbersOfBackpacks, sizeof(int));
	
	// No one object is saved to a backpack.
	(*node)->solution = (int*) malloc (sizeof(int) * numbersOfObjects);
	int i;
	for (i = 0; i < numbersOfObjects; i++)
		(*node)->solution[i] = -1;
}

// Free a node struct allocted memory. 
int destroySolutionSpace(struct Node *node) {		
	free(node->solution);
	free(node->backpacksWeights);
	free(node);
}

// Alocate memory for tasks array.
struct Node** createTasksSpace(int numbersOfBackpacks) {
	// The array size is: backpacks ^ levels to generate. 
	int tamanoMaximoLista = raiseTo(numbersOfBackpacks, LEVELS_TO_GENERATE);
	// Array pointer.
	return (struct Node**) malloc(sizeof(struct Node*) * tamanoMaximoLista);		
}

// Free tasks array allocted memory. 
void freeTasksSpace(int generatedTasks, struct Node **tasksList) {
	int i;
	
	// All generated nodes are freed.
	for (i = 0; i < generatedTasks; i++) {
		destroySolutionSpace(tasksList[i]);	
	}
	
	free (tasksList);	// Tasks array is freed.
}

// Copy a node 'nodoOrigen' to 'nodoDestino'.
int nodeCopy(struct Node *nodoOrigen, struct Node *nodoDestino, int numbersOfObjects, int numbersOfBackpacks) {
	// Copy of actual solution.
	memcpy(nodoDestino->solution, nodoOrigen->solution, sizeof(int) * numbersOfObjects);
	// Copy of backpacks weights.
	memcpy(nodoDestino->backpacksWeights, nodoOrigen->backpacksWeights, sizeof(int) * numbersOfBackpacks);
	// Copy of level.
	nodoDestino->level = nodoOrigen->level;
}

// Inicializate a node saving the actual object (level) to the specified backpack (mochila).
void InitializeNode(struct Node * node, int level, int mochila, int *objectsWeights) {
	// Lelvel is specified.
	node->level = level;	
	
	// If a object where in this position, his weight is subtracted.
	if (node->solution[ node->level ] != -1) {
		node->backpacksWeights[ node->solution[ node->level ] ] -= objectsWeights[ node->level ];
	}

	// Put the actual object (level) on the specified backpack. 
	node->solution[ node->level ] = mochila;
	// His weight is added to the backpack.
	node->backpacksWeights[ node->solution[ node->level ] ] += objectsWeights[ node->level ];
	// Level is increased. The object (level) is saved.
	node->level++;
}

// Generate node of the specified level. If level is target level, the node is saved in tasks array. Otherwise, childrens are generated.
void createTasks(struct Node * raiz, struct Node** tasksList, int nivelActual, 
				 int nivelesGenerar, int numbersOfBackpacks, int numbersOfObjects, int *objectsWeights, int *backpacksCapacities, int *generatedTasks) {
	// Si no level must be generated, return.
	if (nivelesGenerar == 0) {
		return;
	}
	// If target level is over or equal to the object numbers, levels generated are object - 1.
	else if (nivelesGenerar >= numbersOfObjects) {
		nivelesGenerar = numbersOfObjects - 1;
	}
	
	// Generation tasks variables.
	struct Node *actualNode, *lastNode;								// ActualNode and LastNodeGenerated
	createSolutionSpace(numbersOfBackpacks, numbersOfObjects, &actualNode);	// Memory allocated.
	nodeCopy(raiz, actualNode, numbersOfObjects, numbersOfBackpacks);		// Copy of root node into actualNode.
	
	int m;	
	// For every backpack.
	for (m = 0; m < numbersOfBackpacks; m++) {
		// Node is generated. Object (level) is saved into next backpack.
		generate(actualNode, objectsWeights); 
		
		// If node meets criteria, this node can be saved as task or ready to generate childrens.
		if (criteria(actualNode, numbersOfObjects, backpacksCapacities)) {
			// ActualNode is now LastNode.
			lastNode = actualNode;
			createSolutionSpace(numbersOfBackpacks, numbersOfObjects, &actualNode);		// Memory is allocated.
			nodeCopy(lastNode, actualNode, numbersOfObjects, numbersOfBackpacks);   	// Node copy.
			
			// Level is increased.
			lastNode->level++;					
			
			// If level generated is not the target level. Childrens are generated by recursivity.
			if (nivelActual + 1 < nivelesGenerar) {
				createTasks(lastNode, tasksList, nivelActual+1, nivelesGenerar, numbersOfBackpacks, numbersOfObjects, objectsWeights, backpacksCapacities, generatedTasks);
			// Otherwise, this node is saved in task list.
			} else if (nivelActual + 1 == nivelesGenerar) {			
				// Node is saved as task. Array index is increased.
				tasksList[*generatedTasks] = lastNode;					
				(*generatedTasks)++;
			}
		}
	}
}

// Tasks are generated in parallel. 
void generateTasks(int numbersOfBackpacks, int numbersOfObjects, int *backpacksCapacities, int *objectsWeights, int *generatedTasks, struct Node **tasksList) {
	struct Node *node;
	createSolutionSpace(numbersOfBackpacks, numbersOfObjects, &node);		
	
	int i;
	#pragma omp for
	for (i = 0; i < numbersOfBackpacks; i++) {
		// First level node is generated.
		InitializeNode(node, 0, i, objectsWeights);		
		// Childrens are generated.
		createTasks(node, tasksList, 1, LEVELS_TO_GENERATE, numbersOfBackpacks, numbersOfObjects, objectsWeights, backpacksCapacities, generatedTasks);
	}
	
	// Generated node is freed.
	destroySolutionSpace(node);	
}

// Sequential Backtracking for a node.
void backtracking(struct Node *node, int *bestPersonalValue, int numbersOfBackpacks, int numbersOfObjects, int *backpacksCapacities, int *objectsWeights, int *affinity) {
	// Initial level of the node.
	int initialLevel = node->level;
	
	do {
		generate(node, objectsWeights); 
	
		// If node is a solution. His value is calculated and it's compared with best value.
		if (isSolution(node, numbersOfObjects, backpacksCapacities)) {
			int value = calculateValue(numbersOfObjects, node, affinity);
			if (value > *bestPersonalValue) {
				*bestPersonalValue = value;
			}
		}
		
		// If this node meets the criteria, level is increased.
		if (criteria(node, numbersOfObjects, backpacksCapacities)) {
			node->level++;
		}
		// Otherwhise, while level is diferent of initial task level and the node has no brothers (more backpacks to save the actual objet), level is decreased. 
		else {
			while (node->level >= initialLevel && !hasMoreBrothers(node, numbersOfBackpacks)) {
				goBack(node, numbersOfBackpacks, objectsWeights);
			}
		}
	// While initial level is not reached.
	} while (node->level >= initialLevel);
}

// Compute all task of tasks array in parallel. 
// int actualTask is shared tasks array index.
void computeTasks(int numbersOfBackpacks, int numbersOfObjects, int *backpacksCapacities, int *objectsWeights, int *affinity,
					int *actualTask, int *bestPersonalValue, int generatedTasks, struct Node **tasksList) {
	
	/** Private variables to access to the tasks array. */
	int reservedNodesUsed = NUM_RESERVED_NODES;	// Nodes reserved used. Initially, same as constant nodes reserved.
	int actualTaskIndex;							// Private tasks array index.
	
	int i;
	#pragma omp for schedule(dynamic, NUM_RESERVED_NODES) 
	// For every task generated.
	for (i = 0; i < generatedTasks; i++) {
		// If all reserved nodes are used.
		if (reservedNodesUsed == NUM_RESERVED_NODES) {
			// Shared tasks array index is updated under critical block.
			#pragma omp critical(reservaTareas)
			{
				actualTaskIndex = (*actualTask);	// Personal tasks array index updated.
				(*actualTask) += NUM_RESERVED_NODES;	// Shared tasks array index updated.
			}		
			reservedNodesUsed = 1;			// Initially only one node is used.
		}
		// Otherwise, reserved nodes remaining. Only private variables are updated.
		else {
			actualTaskIndex++;			// Personal tasks array index updated.
			reservedNodesUsed++;		// Used nodes updated.
		}		
		
		// If index is not over generated taks.
		if (actualTaskIndex < generatedTasks) {
			// Sequential Backtracking is applied to the extracted node of tasks array.
			backtracking(tasksList[actualTaskIndex], bestPersonalValue, numbersOfBackpacks, numbersOfObjects, backpacksCapacities, objectsWeights, affinity);
		}
	}
}

// Compare si the personal best actual value is betther than the global best value. 
int setLocalBestValue(int bestPersonalValue, int *result) {
	// The best value is saved into global value.
	#pragma omp critical(VOAGLobal)
	if (bestPersonalValue > (*result)) {
		(*result) = bestPersonalValue;
	}
}

int sec(int numbersOfBackpacks, int numbersOfObjects, int *backpacksCapacities, int *objectsWeights, int *affinity) {
	// Num threads, same as jupiter cores with no HyperThreading.
	omp_set_num_threads(12);
	
	/** Shared Variables */
	struct Node **tasksList = createTasksSpace(numbersOfBackpacks);	// Tasks array.
	int generatedTasks = 0; // Total of generated tasks.
	int actualTask = 0;	 // Shared variable of first task not assigned.
	int result = 0;		 // Return value, best value found.
	
	#pragma omp parallel
	{
		int bestPersonalValue = 0;	// Thread personal best actual value.	
		// Tasks are generated in parallel.
		generateTasks(numbersOfBackpacks, numbersOfObjects, backpacksCapacities, objectsWeights, &generatedTasks, tasksList);
		// Tasks are computed in parallel.
		computeTasks(numbersOfBackpacks, numbersOfObjects, backpacksCapacities, objectsWeights, affinity, &actualTask, &bestPersonalValue, generatedTasks, tasksList);
		// Private best value is set into shared variable.
		setLocalBestValue(bestPersonalValue, &result);
	}
	
	// Tasks array is freed. 
	freeTasksSpace(generatedTasks, tasksList);
	
	return result;
}
