#include <stdio.h>
#include <stdlib.h>

typedef struct Edge
{
    int v;
    int w;
    int weight;
} Edge;

void bellman_ford(Edge** graph, int N, int M)
{
    int dist[N+1];
    int prev[N+1];
    for (int i = 1; i <= N; ++i)
    {
        dist[i] = 30000;
        prev[i] = -1;
    }
    dist[1] = 0;
    for (int i = 1; i <= N - 1; ++i)
    {
        for (int j = 0; j < M; ++j)
        {
            int v = (*graph)[j].v;
            int w = (*graph)[j].w;
            if (dist[w] > dist[v] + (*graph)[j].weight)
            {
                if (dist[v] != 30000)
                {
                    dist[w] = dist[v] + (*graph)[j].weight;
                    prev[w] = v;
                }
            }
        }
    }
    for (int i = 1; i <= N; ++i)
    {
        printf("%d ", dist[i]);
    }
}

int main()
{
    int N, M, v, w, weight;
    scanf("%d%d", &N, &M);
    Edge* graph = (Edge*)malloc(M*sizeof(Edge));
    for (int i = 0; i < M; ++i)
    {
        scanf("%d%d%d", &v, &w, &weight);
        graph[i].v = v;
        graph[i].w = w;
        graph[i].weight = weight;
    }
    bellman_ford(&graph, N, M);
    free(graph);
    return 0;
}
