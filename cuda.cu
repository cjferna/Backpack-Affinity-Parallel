/*
  CPP_CONTEST=2013Cal-2011
  CPP_PROBLEM=S
  CPP_LANG=CUDA
  CPP_PROCESSES_PER_NODE=saturno 1
*/

/* RECORD
Carlos Javier Fernández Candel
student of Methodology of Parallel Programming
at the University of Murcia
February 1, 2016
time 8705 msec
speedup 1.33
The improvement is residual,
but it is included in the table records
because it is the first CUDA implementation*/

/*
 Tasks list algorithm scheme is used.
 Nodes are generated and saved on a list in parallel until level specified('LEVELS_TO_GENERATE'). That node list is 
 computed in parallel, where every thread takes as nodes as ('NUM_RESERVED_NODES'), to prevent critical waste time. 
 Finally, all threads tries to put his personal best result 'VOA' as global better result.
*/

#include <stdlib.h>
#include <string.h>

#define LEVELS_TO_GENERATE 4 	// Numbers of levels to generate at start to balance compute distribution.
#define NODES_PER_THREAD 15		// Numbers of nodes per CUDA thread.

// Represent a node of solution.
struct Node {
	int *solution;			// solution[x]. Every posistion save the backpack where obejct 'x' is saved.
	int *backpacksWeights;	// backpacksWeights[x]. Weight of every backpack. Weight of backpack 'x'.
	int level;				// Actual level. Actual ojbect.
};

// Envolve a node. Put the actual object (level) on the next backpack.
__device__ void generate(struct Node *node, int *objectsWeights) {
	// If a object where in this position, his weight is subtracted.
	if (node->solution[ node->level ] != -1) {
		node->backpacksWeights[ node->solution[ node->level ] ] -= objectsWeights[ node->level ];
	}
	
	// Put the actual object (level) on the next backpack. His weight is added to the backpack.
	node->solution[ node->level ]++;
	node->backpacksWeights[ node->solution[ node->level ] ] += objectsWeights[ node->level ];
}

// Determine if node is a complete solution.
__device__ void isSolution(struct Node *node, int numbersOfObjects, int *backpacksCapacities, int *result) {
	// If it's last level. level == last level == last object.
	// And if the weight of the backpack where te actual object is saved (last level) is not over the backpack capacity. (Backpack where object is saved :solution[ level ] ]) 
	*result = ((node->level == numbersOfObjects - 1) 
		   && node->backpacksWeights[ node->solution[ node->level ] ] <= backpacksCapacities[ node->solution[ node->level ] ]);
}

// Value of this solution is calculated.
__device__ void calculateValue(int numbersOfObjects, struct Node *node, int *affinity, int *result) {
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
	
	*result = value;
}

// Determine  if a node can grown to the next level. 
__device__ void criteria(struct Node *node, int numbersOfObjects, int *backpacksCapacities, int *result) {
	// If the levels is not the last level.
	// And if the weight of the backpack where last object where saved is not over the capacity of the backpack.
	*result = (node->level != numbersOfObjects - 1 && node->backpacksWeights[ node->solution[ node->level ] ] <= backpacksCapacities[ node->solution[ node->level ] ]);
}

// Determine if this level has more brothers. 
__device__ void hasMoreBrothers(struct Node *node, int numbersOfBackpacks, int *result) {
	// If all backpacks are not used to try to save the current object (level).
	*result = (node->solution[ node->level ] < numbersOfBackpacks - 1);
}

// Deletes the actual object. Node level will be reduced.
__device__ void goBack(struct Node *node, int numbersOfBackpacks, int *objectsWeights) {
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
void initializeSolutionSpace(int numbersOfBackpacks, int numbersOfObjects, struct Node *node) {	
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

// Memory is allocated to save an solution(node). Objets assing array and Backpacks weights array.
void createSolutionSpace(int numbersOfBackpacks, int numbersOfObjects, struct Node **node) {
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
void destroySolutionSpace(struct Node *node) {		
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
void nodeCopy(struct Node *nodoOrigen, struct Node *nodoDestino, int numbersOfObjects, int numbersOfBackpacks) {
	// Copy of actual solution.
	memcpy(nodoDestino->solution, nodoOrigen->solution, sizeof(int) * numbersOfObjects);
	// Copy of backpacks weights.
	memcpy(nodoDestino->backpacksWeights, nodoOrigen->backpacksWeights, sizeof(int) * numbersOfBackpacks);
	// Copy of level.
	nodoDestino->level = nodoOrigen->level;
}

// Inicializate a node saving the actual object (level) to the specified backpack (mochila).
void InitializeNode(struct Node * node, int level, int backpack, int *objectsWeights) {
	// Lelvel is specified.
	node->level = level;	
	
	// If a object where in this position, his weight is subtracted.
	if (node->solution[ node->level ] != -1) {
		node->backpacksWeights[ node->solution[ node->level ] ] -= objectsWeights[ node->level ];
	}

	// Put the actual object (level) on the specified backpack. 
	node->solution[ node->level ] = backpack;
	// His weight is added to the backpack.
	node->backpacksWeights[ node->solution[ node->level ] ] += objectsWeights[ node->level ];
	// Level is increased. The object (level) is saved.
	node->level++;
}

void generate_generate(struct Node *node, int *objectsWeights) {
	// If a object where in this position, his weight is subtracted.
	if (node->solution[ node->level ] != -1) {
		node->backpacksWeights[ node->solution[ node->level ] ] -= objectsWeights[ node->level ];
	}
	
	// Put the actual object (level) on the next backpack. His weight is added to the backpack.
	node->solution[ node->level ]++;
	node->backpacksWeights[ node->solution[ node->level ] ] += objectsWeights[ node->level ];
}

// Determine  if a node can grown to the next level. 
int generate_criteria(struct Node *node, int numbersOfObjects, int *backpacksCapacities) {
	// If the levels is not the last level.
	// And if the weight of the backpack where last object where saved is not over the capacity of the backpack.
	return (node->level != numbersOfObjects - 1 && node->backpacksWeights[ node->solution[ node->level ] ] <= backpacksCapacities[ node->solution[ node->level ] ]);
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
		generate_generate(actualNode, objectsWeights); 
		
		// If node meets criteria, this node can be saved as task or ready to generate childrens.
		if (generate_criteria(actualNode, numbersOfObjects, backpacksCapacities)) {
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
__device__ void backtracking(struct Node *node, int *bestPersonalValue, int numbersOfBackpacks, int numbersOfObjects, int *backpacksCapacities, int *objectsWeights, int *affinity) {
	// Initial level of the node.
	int initialLevel = node->level;
	
	do {
		generate(node, objectsWeights); 

		// If node is a solution. His value is calculated and it's compared with best value.
		int isSolutionVar;
		isSolution(node, numbersOfObjects, backpacksCapacities, &isSolutionVar);
		if (isSolutionVar) {
			int value;
			calculateValue(numbersOfObjects, node, affinity, &value);
			if (value > *bestPersonalValue) {
				*bestPersonalValue = value;
			}
		}
		
		// If this node meets the criteria, level is increased.
		int criteriaVar = 0;
		criteria(node, numbersOfObjects, backpacksCapacities, &criteriaVar);
		
		if (criteriaVar) {
			node->level++;
		}
		// Otherwhise, while level is diferent of initial task level and the node has no brothers (more backpacks to save the actual objet), level is decreased. 
		else {
			int more;
			hasMoreBrothers(node, numbersOfBackpacks, &more);
			
			while (node->level >= initialLevel && !more) {
				goBack(node, numbersOfBackpacks, objectsWeights);
				hasMoreBrothers(node, numbersOfBackpacks, &more);
			}
		}
	// While initial level is not reached.
	} while (node->level >= initialLevel);
}

// Compute all task of tasks array in parallel. 
__global__ void computeTasks(int numbersOfBackpacks, int numbersOfObjects, int *backpacksCapacities, int *objectsWeights, int *affinity,
					int generatedTasks, struct Node **tasksList, int *topValues) {
	int idx = blockIdx.x * blockDim.x + threadIdx.x;		

	if (idx  < generatedTasks) {
		tasksList[idx]->level = LEVELS_TO_GENERATE;
		topValues[idx] = 0;
		backtracking(tasksList[idx], &topValues[idx], numbersOfBackpacks, numbersOfObjects, backpacksCapacities, objectsWeights, affinity);
	}
}

void setCudaMemory(int **dev_backpacksCapacities, int **dev_objectsWeights, int **dev_affinity, int *backpacksCapacities, 
				   int *objectsWeights, int *affinity, int numbersOfBackpacks, int numbersOfObjects) {
	// Memory Set.	
	cudaMalloc((void**)dev_backpacksCapacities, numbersOfBackpacks * sizeof(int));	
	cudaMalloc((void**)dev_objectsWeights, numbersOfObjects * sizeof(int));	
	cudaMalloc((void**)dev_affinity, numbersOfObjects * numbersOfObjects * sizeof(int));
	
	// Copies	
	cudaMemcpy(*dev_backpacksCapacities, backpacksCapacities, numbersOfBackpacks * sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(*dev_objectsWeights, objectsWeights, numbersOfObjects * sizeof(int), cudaMemcpyHostToDevice);
	cudaMemcpy(*dev_affinity, affinity, numbersOfObjects * numbersOfObjects * sizeof(int), cudaMemcpyHostToDevice);
}

void setAndCopyTaskListToCudaMemory(struct Node ***dev_tasksList, struct Node **h_tasksList, struct Node **tasksList, struct Node **h_tasksList_to_free, int generatedTasks,
									int numbersOfBackpacks, int numbersOfObjects) {
	cudaMalloc((void**)dev_tasksList, sizeof(struct Node*) * generatedTasks);
	
	int i;
	for(i = 0; i < generatedTasks; i++) {
		// Struct Memory.
		struct Node *nodo = (struct Node*) malloc(sizeof(struct Node));	
		h_tasksList_to_free[i] = nodo;
		
		// BackPackWeights Memory.
		cudaMalloc((void**) &nodo->backpacksWeights, numbersOfBackpacks * sizeof(int));
		cudaMemcpy(nodo->backpacksWeights, tasksList[i]->backpacksWeights, numbersOfBackpacks * sizeof(int), cudaMemcpyHostToDevice);
		
		// Solution Memory.
		cudaMalloc((void**) &nodo->solution, numbersOfObjects * sizeof(int));
		cudaMemcpy(nodo->solution, tasksList[i]->solution, numbersOfObjects * sizeof(int), cudaMemcpyHostToDevice);
		
		
		cudaMalloc((void**)&h_tasksList[i], sizeof(struct Node));
		cudaMemcpy(h_tasksList[i], nodo, sizeof(struct Node), cudaMemcpyHostToDevice);
	}
	
	cudaMemcpy(*dev_tasksList, h_tasksList, sizeof(struct Node*) * generatedTasks, cudaMemcpyHostToDevice);
}


// Compare si the personal best actual value is betther than the global best value. 
void setLocalBestValue(int *result, int *h_topValues, int generatedTasks) {
	int i;
	for(i = 0; i < generatedTasks; i++) {
		if ((*result) < h_topValues[i]) {
			(*result) = h_topValues[i];
		}
	}
}

void freeCudaMemory(int *dev_topValues, struct Node **dev_tasksList, int *dev_backpacksCapacities, int *dev_objectsWeights, int *dev_affinity, 
					struct Node **h_tasksList, struct Node **h_tasksList_to_free, int *h_topValues, int generatedTasks) {
	int i;
	for(i = 0; i < generatedTasks; i++) {
		cudaFree(h_tasksList_to_free[i]->backpacksWeights);
		cudaFree(h_tasksList_to_free[i]->solution);
		cudaFree(h_tasksList[i]);
	}
	
	cudaFree(dev_topValues);
	cudaFree(dev_tasksList);
	cudaFree(dev_backpacksCapacities);
	cudaFree(dev_objectsWeights);
	cudaFree(dev_affinity);
	
	free(h_tasksList_to_free);
	free(h_tasksList);
	free(h_topValues);
}

int sec(int numbersOfBackpacks, int numbersOfObjects, int *backpacksCapacities, int *objectsWeights, int *affinity) {
	
/** Generate Tasks*/ 
	struct Node **tasksList = createTasksSpace(numbersOfBackpacks);	// Tasks array.
	int generatedTasks = 0;
	generateTasks(numbersOfBackpacks, numbersOfObjects, backpacksCapacities, objectsWeights, &generatedTasks, tasksList);

/** CUDA Memory and Copies */
	int *dev_backpacksCapacities = 0, *dev_objectsWeights = 0, *dev_affinity = 0;	
	setCudaMemory(&dev_backpacksCapacities, &dev_objectsWeights, &dev_affinity, backpacksCapacities, objectsWeights, affinity, numbersOfBackpacks, numbersOfObjects);
	
	struct Node **dev_tasksList = 0, **h_tasksList = (struct Node**) malloc(sizeof(struct Node*) * generatedTasks);
	struct Node **h_tasksList_to_free = (struct Node**) malloc(sizeof(struct Node*) * generatedTasks);
	
	setAndCopyTaskListToCudaMemory(&dev_tasksList, h_tasksList, tasksList, h_tasksList_to_free, generatedTasks, numbersOfBackpacks, numbersOfObjects);	
	int *dev_topValues = 0, *h_topValues = (int *) malloc(generatedTasks * sizeof(int));	
	cudaMalloc((void**)&dev_topValues, generatedTasks * sizeof(int));
	
/** Tasks compute */
	int numThreads = NODES_PER_THREAD, numBlocks = (generatedTasks / NODES_PER_THREAD) +1;
	computeTasks<<<numBlocks, numThreads, 0>>>(numbersOfBackpacks, numbersOfObjects, dev_backpacksCapacities, dev_objectsWeights, dev_affinity, generatedTasks,
											   dev_tasksList, dev_topValues);
/* Result Set */
	int result = 0;
	cudaMemcpy(h_topValues, dev_topValues, generatedTasks * sizeof(int), cudaMemcpyDeviceToHost);
	setLocalBestValue(&result, h_topValues, generatedTasks);

/* Memory Free */
	freeCudaMemory(dev_topValues, dev_tasksList, dev_backpacksCapacities, dev_objectsWeights, dev_affinity,h_tasksList, h_tasksList_to_free, h_topValues, generatedTasks);	
	freeTasksSpace(generatedTasks, tasksList);
	
	return result;
}
