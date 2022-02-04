/*
  CPP_CONTEST=2013Cal-2011
  CPP_PROBLEM=Q
  CPP_LANG=C+OPENMP
  CPP_PROCESSES_PER_NODE=saturno 1
*/

//

#include <stdlib.h>
#include <omp.h>
 
void generar(int nivel,int *peso,int *solucion,int *pesopormochila)
{
  if(solucion[nivel]!=-1)
  {
    pesopormochila[solucion[nivel]]-=peso[nivel];
  }
  solucion[nivel]++;  
  pesopormochila[solucion[nivel]]+=peso[nivel];
}

int essolucion(int nivel,int objetos,int *capacidad,int *pesopormochila,int *solucion)
{
  return((nivel==objetos-1) && pesopormochila[solucion[nivel]]<=capacidad[solucion[nivel]]);
}
    
int calcularvalor(int objetos,int *solucion,int *afinidad)
{
  int valor=0,i,j;

  for(i=0;i<objetos;i++)
  {
    for(j=i+1;j<objetos;j++)
    {
      if(solucion[i]==solucion[j])
      {
        valor+=(afinidad[i*objetos+j]+afinidad[j*objetos+i]);
      }
    }
  }
  return valor;
}

int criterio(int nivel,int objetos,int *capacidad,int *solucion,int *pesopormochila)
{
  return(nivel!=objetos-1 && pesopormochila[solucion[nivel]]<=capacidad[solucion[nivel]]);
}

int mashermanos(int nivel,int mochilas,int *solucion)
{
  return(solucion[nivel]<mochilas-1);
}

void retroceder(int nivel,int mochilas,int *peso,int *solucion,int *pesopormochila)
{
  pesopormochila[mochilas-1]-=peso[nivel];
  solucion[nivel]=-1;  
}

/*void escribir(int nivel,int mochilas,int objetos,int *capacidad,int *peso,int *afinidad,int *solucion,int *pesopormochila)
{
  int i,seguir;

  printf("En nivel %d\n",nivel);
  printf("  solucion:");
  for(i=0;i<objetos;i++)
    printf(" %d ",solucion[i]);
  printf("\n");
  printf("  pesos por mochila:");
  for(i=0;i<mochilas;i++)
    printf(" %d ",pesopormochila[i]);
  printf("\n");
  printf("\n");
}*/

int sec(int mochilas,int objetos,int *capacidad,int *peso,int *afinidad)
{
  int *solucion,*pesopormochila,valor=0,VOA=0,i,nivel=0;

  solucion=(int *) malloc(sizeof(int)*objetos);
  pesopormochila=(int *) malloc(sizeof(int)*mochilas);

  for(i=0;i<objetos;i++)
    solucion[i]=-1;

  for(i=0;i<mochilas;i++)
    pesopormochila[i]=0;

  //printf("Tras Inicializar\n");
  //escribir(nivel,mochilas,objetos,capacidad,peso,afinidad,solucion,pesopormochila);
  do{
    generar(nivel,peso,solucion,pesopormochila);
  //printf("Tras Generar\n");
  //escribir(nivel,mochilas,objetos,capacidad,peso,afinidad,solucion,pesopormochila);
    if(essolucion(nivel,objetos,capacidad,pesopormochila,solucion)==1)
    {
      valor=calcularvalor(objetos,solucion,afinidad);
      if(valor>VOA)
      {  VOA=valor;
  //printf("Tras Essolucion, valor %d\n",valor);
  //escribir(nivel,mochilas,objetos,capacidad,peso,afinidad,solucion,pesopormochila);
      }
    }
    if(criterio(nivel,objetos,capacidad,solucion,pesopormochila)==1)
    {
  //printf("Tras Criterio\n");
  //escribir(nivel,mochilas,objetos,capacidad,peso,afinidad,solucion,pesopormochila);
      nivel++;
    }
    else
    {
      while(nivel!=-1 && mashermanos(nivel,mochilas,solucion)==0)
      {
  //printf("Tras Mashermanos\n");
  //escribir(nivel,mochilas,objetos,capacidad,peso,afinidad,solucion,pesopormochila);
        retroceder(nivel,mochilas,peso,solucion,pesopormochila);
        nivel--;
  //printf("Tras Retroceder\n");
  //escribir(nivel,mochilas,objetos,capacidad,peso,afinidad,solucion,pesopormochila);
      }
    }
  } while(nivel!=-1);

  free(solucion);
  free(pesopormochila);
  return VOA;
}
