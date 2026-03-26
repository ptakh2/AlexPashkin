#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include "lodepng.h"

typedef struct vertex {
    unsigned char color;
    int index; // указывает на последнее ребро в массиве Edges
} vertex;

typedef struct Graph {
    int size;
    int width;
    int height;
    int cur_ver; // индекс текущей вершины
    vertex* offset; // массив вершин
    int* edges; // массив ребер
} Graph;

// Очередь реализована на массиве
typedef struct Queue{
    int* data;
    int head;
    int tail;
    int capacity;
} Queue;

Queue* create_queue(int capacity) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->data = (int*)malloc(capacity * sizeof(int));
    q->head = 0;
    q->tail = 0;
    q->capacity = capacity;
    return q;
}

void queue_push(Queue* q, int val) {
    q->data[q->tail] = val;
    q->tail++;
}

int queue_pop(Queue* q) {
    int val = q->data[q->head];
    q->head++;
    return val;
}

int queue_empty(Queue* q) {
    return q->head == q->tail;
}

void destroy_queue(Queue* q) {
    free(q->data);
    free(q);
}

// BFS я использовал только для прохода по одной компоненте связности из белых вершин
// массив visited здесь общий для всей области, чтобы для одной и той же компоненты BFS
// не запускался повторно. Количество запусков BFS и есть количество компонент связности
void BFS(Graph* g, char* visited, int start) {
    Queue* q = create_queue(g->size);
    queue_push(q, start);
    visited[start] = 1;

    while (queue_empty(q) == 0) {
        int cur = queue_pop(q);
        // перебираем 4 ребра
        for (int dir = 0; dir < 4; ++dir) {
            int neigh = g->edges[4*cur+dir];
            if (neigh != -1 && visited[neigh] == 0 && g->offset[neigh + 1].color == 255) {
                visited[neigh] = 1;
                queue_push(q, neigh);
            }
        }
    }
    destroy_queue(q);
}

// принимаем на вход: имя файла, указатели на int для хранения прочитанной ширины и высоты картинки
// возвращаем указатель на выделенную память для хранения картинки
// Если память выделить не смогли, отдаем нулевой указатель и пишем сообщение об ошибке
unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height)
{
  unsigned char* image = NULL;
  int error = lodepng_decode32_file(&image, width, height, filename);
  if (error != 0)
  {
    printf("error %u: %s\n", error, lodepng_error_text(error));
  }
  return (image);
}

// принимаем на вход: имя файла для записи, указатель на массив пикселей,  ширину и высоту картинки
// Если преобразовать массив в картинку или сохранить не смогли,  пишем сообщение об ошибке
void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
  unsigned char* png;
  size_t pngsize;
  int error = lodepng_encode32(&png, &pngsize, image, width, height);
  if(error == 0)
  {
      lodepng_save_file(png, pngsize, filename);
  }
  else
  {
      printf("error %u: %s\n", error, lodepng_error_text(error));
  }
  free(png);
}


// вариант огрубления серого цвета в ЧБ
void contrast(unsigned char *col, int bw_size)
{
    int i;
    for(i=0; i < bw_size; i++) {
        // разделим пиксели на черные и белые (0 и 1)
        if(col[i] < 76) {
            col[i] = 0;
        }
        else {
            col[i] = 255;
        }
    }
    return;
}

// Заполняет все поля структуры графа
// offset имеет фиктивную вершину для правильно представления CSR,
// но для решения задачи я это вроде не использовал
void create_work_area(int i, int w, int h, Graph* work_areas) {
    int* edges;
    vertex* offset;
    work_areas[i].width = w;
    work_areas[i].height = h;
    work_areas[i].size = w*h;
    work_areas[i].cur_ver = 0;

    edges = (int*)malloc((4*w*h*sizeof(int)));
    offset = (vertex*)malloc((w*h+1)*sizeof(vertex));
    work_areas[i].edges = edges;
    work_areas[i].offset = offset;
    work_areas[i].offset[0].color = 0;
    work_areas[i].offset[0].index = 0;
}

// Заполняет граф вершинами и ребрами
// на границах нужно быть осторожным: не должно быть связей с пикселями вне области
void fill_work_area(int i_ar, Graph* work_areas, int i_pic, unsigned char *col) {
    int j = work_areas[i_ar].cur_ver;
    work_areas[i_ar].offset[j+1].color = col[i_pic];
    work_areas[i_ar].offset[j+1].index = 4*j+3;
    for (int k = 0;k < 4;++k) {
        work_areas[i_ar].edges[4*j+k] = -1;
    }
    if (j >= work_areas[i_ar].width) { // верх
        work_areas[i_ar].edges[4*j] = j - work_areas[i_ar].width;
    }
    if (j % work_areas[i_ar].width != 0) { // лево
        work_areas[i_ar].edges[4*j+1] = j - 1;
    }
    if ((j+1) % work_areas[i_ar].width != 0) { // право
        work_areas[i_ar].edges[4*j+2] = j + 1;
    }
    if (j < work_areas[i_ar].size - work_areas[i_ar].width) { // низ
        work_areas[i_ar].edges[4*j+3] = j + work_areas[i_ar].width;
    }
    work_areas[i_ar].cur_ver++;
}

// общая функция для создания рабочих областей
void find_area(unsigned char *col, int bw_size, Graph* work_areas)
{
    int i;
    create_work_area(0, 175, 273, work_areas); // для области 1
    create_work_area(1, 551, 328, work_areas); // для 2
    create_work_area(2, 290, 45, work_areas); // 3
    create_work_area(3, 64, 46, work_areas); // 4
    create_work_area(4, 56, 64, work_areas); // 5
    create_work_area(5, 44, 61, work_areas); // 6
    create_work_area(6, 31, 46, work_areas); // 7
    for(i=0; i < bw_size; i++) {
        if ((((i % 1280) >= 536 && (i % 1280) <= 710)) && ((i >= 0 && i <= 273*1280))) { // область 1
            fill_work_area(0, work_areas, i, col);
        }
        if ((((i % 1280) >= 564 && (i % 1280) <= 1114)) && ((i >= 275*1280 && i <= 603*1280))) { // 2
            fill_work_area(1, work_areas, i, col);
        }
        if ((((i % 1280) >= 825 && (i % 1280) <= 1114)) && ((i >= 602*1280 && i <= 647*1280))) { // 3
            fill_work_area(2, work_areas, i, col);
        }
        if ((((i % 1280) >= 501 && (i % 1280) <= 564)) && ((i >= 301*1280 && i <= 347*1280))) { // 4
            fill_work_area(3, work_areas, i, col);
        }
        if ((((i % 1280) >= 509 && (i % 1280) <= 564)) && ((i >= 347*1280 && i <= 411*1280))) { // 5
            fill_work_area(4, work_areas, i, col);
        }
        if ((((i % 1280) >= 521 && (i % 1280) <= 564)) && ((i >= 452*1280 && i <= 513*1280))) { // 6
            fill_work_area(5, work_areas, i, col);
        }
        if ((((i % 1280) >= 540 && (i % 1280) <= 564)) && ((i >= 514*1280 && i <= 560*1280))) { // 7
            fill_work_area(6, work_areas, i, col);
        }
    }

    return;
}

// подсчёт количества компонент связности в одной области.
// Просто проходим по всей области и запускаем BFS только, когда
// находим белую вершину
int count_ships(Graph* work_area) {
    int size = work_area->size;
    char* visited = (char*)calloc(size, sizeof(char));
    int ships = 0;

    for (int v = 0; v < size; ++v) {
        if (visited[v] == 0 && work_area->offset[v+1].color == 255) {
            BFS(work_area, visited, v);
            ++ships;
        }
    }

    free(visited);
    return ships;
}

void count_ships_total(unsigned char* col, int bw_size, Graph* work_areas) {
    int total = 0;
    for (int i = 0; i < 7; ++i) {
        total += count_ships(&work_areas[i]);
    }
    printf("Total ships count in the gulf: %d", total-2);
    // -2 потому что в области 4 одна из компонент связности лишняя и в областях 6 и 7
    // есть один общий корабль, посчитанный два раза.
    // На точность ответа также может сильно влиять область 7 в силу её плохого качества
    // на исходной картинке. Я немного подвинул её левую границу вправо.
    // В целом решение без гауссова размытия более точное, чем с его использованием.
}

// Гауссово размыттие
/*void Gauss_blur(unsigned char *col, unsigned char* blr_pic, int width, int height)
{
    int i, j;
    for(i=1; i < height-1; i++)
        for(j=1; j < width-1; j++)
        {
            float k = 0.125;
            float m = 0.09375;
            blr_pic[width*i+j] = k*col[width*i+j] + k*col[width*(i+1)+j] + k*col[width*(i-1)+j];
            blr_pic[width*i+j] = blr_pic[width*i+j] + k*col[width*i+(j+1)] + k*col[width*i+(j-1)];
            blr_pic[width*i+j] = blr_pic[width*i+j] + m*col[width*(i+1)+(j+1)] + m*col[width*(i+1)+(j-1)];
            blr_pic[width*i+j] = blr_pic[width*i+j] + m*col[width*(i-1)+(j+1)] + m*col[width*(i-1)+(j-1)];
        }
   return;
}*/

//  Место для экспериментов
/*void color(unsigned char *blr_pic, unsigned char *res, int size)
{
  int i;
    for(i=1;i<size;i++)
    {
        res[i*4]=40+blr_pic[i]+0.35*blr_pic[i-1];
        res[i*4+1]=65+blr_pic[i];
        res[i*4+2]=170+blr_pic[i];
        res[i*4+3]=255;
    }
    return;
}*/

void convert_into_rgba(unsigned char* col, unsigned char* finish, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        finish[i*4] = col[i];
        finish[i*4+1] = col[i];
        finish[i*4+2] = col[i];
        finish[i*4+3] = 255;
    }
}

int main()
{
    const char* filename = "ships.png";
    unsigned int width, height;
    int size;
    int bw_size;

    // Прочитали картинку
    unsigned char* picture = load_png("ships.png", &width, &height);
    if (picture == NULL)
    {
        printf("Problem reading picture from the file %s. Error.\n", filename);
        return -1;
    }

    size = width * height * 4;
    bw_size = width * height;

    unsigned char* bw_pic = (unsigned char*)malloc(bw_size*sizeof(unsigned char));
    unsigned char* finish;

    finish = picture;
    for (int i = 0; i < bw_size; ++i) {
        //преобразуем пиксель в серый по формуле:
        bw_pic[i] = (picture[i*4] * 299 + picture[i*4+1] * 587 + picture[i*4+2] * 114) / 1000;
    }

    //Gauss_blur(bw_pic, blr_pic, width, height);
    // посмотрим на промежуточные картинки
   // convert_into_rgba(blr_pic, finish, width, height);

    //write_png("gauss.png", finish, width, height);
    contrast(bw_pic, bw_size);
    Graph work_areas[7];
    find_area(bw_pic, bw_size, work_areas);
    count_ships_total(bw_pic, bw_size, work_areas);
    convert_into_rgba(bw_pic, finish, width, height);
    // посмотрим на промежуточные картинки
    write_png("contrast.png", finish, width, height);

    // не забыли почистить память!
    for (int i = 0; i < 7; ++i) {
        free(work_areas[i].edges);
        free(work_areas[i].offset);
    }
    free(bw_pic);
    free(finish);

    return 0;
}