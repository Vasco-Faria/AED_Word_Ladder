//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (work ladder)
//
// Place your student numbers and names here
//   N.Mec. 107323  Name: 107449
//
// Do as much as you can
//   1) MANDATORY: complete the hash table code
//      *) hash_table_create
//      *) hash_table_grow
//      *) hash_table_free
//      *) find_word
//      +) add code to get some statistical data about the hash table
//   2) HIGHLY RECOMMENDED: build the graph (including union-find data) -- use the similar_words function...
//      *) find_representative
//      *) add_edge
//   3) RECOMMENDED: implement breadth-first search in the graph
//      *) breadth_first_search
//   4) RECOMMENDED: list all words belonging to a connected component
//      *) breadth_first_search
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between two words
//      *) breadth_first_search
//      *) path_finder
//      *) test the smallest path from bem to mal
//         [ 0] bem
//         [ 1] tem
//         [ 2] teu
//         [ 3] meu
//         [ 4] mau
//         [ 5] mal
//      *) find other interesting word ladders
//   6) OPTIONAL: compute the diameter of a connected component and list the longest word chain
//      *) breadth_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// static configuration
//

#define _max_word_size_  32
#define _hash_table_load_factor_  0.5

//
// data structures (SUGGESTION --- you may do it in a different way)
//

typedef struct adjacency_node_s  adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s      hash_table_t;

struct adjacency_node_s
{
  adjacency_node_t *next;            // link to the next adjacency list node
  hash_table_node_t *vertex;         // the other vertex
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_];        // the word
  hash_table_node_t *next;           // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;            // head of the linked list of adjancency edges
  int visited;                       // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous;       // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
};

struct hash_table_s
{
  unsigned int hash_table_size;      // the size of the hash table array
  unsigned int number_of_entries;    // the number of entries in the hash table
  unsigned int number_of_edges;      // number of edges (for information purposes only)
  hash_table_node_t **heads;         // the heads of the linked lists
};


//
// allocation and deallocation of linked list nodes (done)
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)malloc(sizeof(adjacency_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}


//
// hash table stuff (mostly to be done)
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if(table[1] == 0u) // do we need to initialize the table[] array?
  {
    unsigned int i,j;

    for(i = 0u;i < 256u;i++)
      for(table[i] = i,j = 0u;j < 8u;j++)
        if(table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while(*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc;
}

static hash_table_t *hash_table_create(void)
{
  hash_table_t *hash_table;
  unsigned int i, size=1024;

//Allocate memory for the hash_table
  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if(hash_table == NULL)
  {
    fprintf(stderr,"create_hash_table: out of memory\n");
    exit(1);
  }

//Modify some of the hash_table values
  hash_table->hash_table_size = size;
  hash_table->number_of_entries = 0;
  hash_table->number_of_edges = 0;

//Allocate memory for the heads array
  hash_table->heads = (hash_table_node_t **)malloc(size * sizeof(hash_table_node_t*));  
  if (hash_table->heads == NULL)
  {
    free(hash_table);
    fprintf(stderr,"create_hash_table->heads: out of memory\n");
    exit(1);
  }

//Initialize the linked list heads to NULL
  for (i = 0; i < size; i++)
    hash_table->heads[i] = NULL;

  return hash_table;
}

static void hash_table_grow(hash_table_t *hash_table)
{
  hash_table_node_t *node, *temp;
  unsigned int i, new_size;

  // Determine the new size of the hash table
  new_size = (unsigned int)(hash_table->hash_table_size / _hash_table_load_factor_);

  // Allocate memory for the new heads array
  hash_table->heads = (hash_table_node_t **)realloc(hash_table->heads,new_size*sizeof(hash_table_node_t*));
  if (hash_table->heads == NULL)
  {
    free(hash_table);
    fprintf(stderr,"create_hash_table->heads: out of memory\n");
    exit(1);
  }

  // Initialize the linked list heads to NULL
  for (i = 0; i < new_size; i++)
    hash_table->heads[i] = NULL;

  // Rehash the entries in the old heads array and insert them into the new heads array
  for (i = 0; i < hash_table->hash_table_size; i++)
  {
    // Rehash each entry in the linked list and insert it into the new heads array
    node = hash_table->heads[i];
    while (node != NULL)
    {
      temp = node;
      node = node->next;

      // Rehash the entry and insert it into the new heads array
      unsigned int new_index = crc32(temp->word) % new_size;
      temp->next = hash_table->heads[new_index];
      hash_table->heads[new_index] = temp;
    }
  }

  // Update the hash table size
  hash_table->hash_table_size = new_size;
}

static void hash_table_free(hash_table_t *hash_table)
{
  hash_table_node_t *node, *temp;
  unsigned int i;

  // Free the memory for each linked list in the hash table
  for (i = 0; i < hash_table->hash_table_size; i++)
  {
    // Free the memory for each node in the linked list
    node = hash_table->heads[i];
    while (node != NULL)
    {
      temp = node;
      free_hash_table_node(temp);
      node = node->next;
    }
  }

  // Free the memory for the heads array and the hash table structure
  free(hash_table->heads); 
  free(hash_table);
}

static hash_table_node_t *find_word(hash_table_t *hash_table,const char *word,int insert_if_not_found)
{
  hash_table_node_t *node;
  unsigned int i;

  i = crc32(word) % hash_table->hash_table_size;

  node = hash_table->heads[i];

  //Find the word given
  while(node != NULL){
    if (strcmp(word,node->word) == 0)
      return node;
    node = node->next;
  }

  //Insert if the word was not found
  if (insert_if_not_found)
  {
    node = allocate_hash_table_node();
    strncpy(node->word,word,_max_word_size_);
    node->next = hash_table->heads[i];
    hash_table->heads[i] = node;
    hash_table->number_of_entries++;
    node->number_of_edges = 1;
    node->number_of_vertices = 1;
    node->representative = node;
  }
  
  return node;
}

//
// add edges to the word ladder graph (Path compression was not done)
//

static hash_table_node_t *find_representative(hash_table_node_t *node)
{
  hash_table_node_t *representative,*next_node;

  representative = node;  
  next_node = node->representative;
  
  while (representative != next_node)
  { 
    representative = next_node;
    next_node = next_node->representative;
  }

  return representative;
}

static void add_edge(hash_table_t *hash_table,hash_table_node_t *from,const char *word)
{
  hash_table_node_t *to,*from_representative,*to_representative;
  adjacency_node_t *link_from, *link_to;

  to = find_word(hash_table,word,0);
  // Check if the destination word exists
  if (to == NULL) return;

  // Check if the edge already exists
  for (link_from = from->head; link_from; link_from = link_from->next)
  {
    if (link_from->vertex == to) return;
  }
  
  // Create adjacency node
  link_from = allocate_adjacency_node();
  link_to = allocate_adjacency_node();

  //Link both words
  link_from->vertex = from;
  link_to->vertex = to;
  link_from->next = to->head;
  link_to->next = from->head;
  to->head = link_from;
  from->head = link_to;

  // Increment the number of edges in the graph
  hash_table->number_of_edges+=2;
 
  // Find representatives of the connected components
  from_representative = find_representative(from);
  to_representative = find_representative(to);
 
  // Check if the 'from' and 'to' vertices belong to different connected components
  if (from_representative != to_representative)
  {
    // Merge the smaller connected component into the bigger one
    if (from_representative->number_of_vertices < to_representative->number_of_vertices)
    {
      from_representative->representative = to_representative;
      to_representative->number_of_vertices += from_representative->number_of_vertices;
      to_representative->number_of_edges += from_representative->number_of_edges;
    }
    else
    {
      to_representative->representative = from_representative;
      from_representative->number_of_vertices += to_representative->number_of_vertices;
      from_representative->number_of_edges += to_representative->number_of_edges;
    } 
  }
}


//
// generates a list of similar words and calls the function add_edge for each one (done)
//
// man utf8 for details on the uft8 encoding
//

static void break_utf8_string(const char *word,int *individual_characters)
{
  int byte0,byte1;

  while(*word != '\0')
  {
    byte0 = (int)(*(word++)) & 0xFF;
    if(byte0 < 0x80)
      *(individual_characters++) = byte0; // plain ASCII character
    else
    {
      byte1 = (int)(*(word++)) & 0xFF;
      if((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
      {
        fprintf(stderr,"break_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
      *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
    }
  }
  *individual_characters = 0; // mark the end!
}

static void make_utf8_string(const int *individual_characters,char word[_max_word_size_])
{
  int code;

  while(*individual_characters != 0)
  {
    code = *(individual_characters++);
    if(code < 0x80)
      *(word++) = (char)code;
    else if(code < (1 << 11))
    { // unicode -> utf8
      *(word++) = 0b11000000 | (code >> 6);
      *(word++) = 0b10000000 | (code & 0b00111111);
    }
    else
    {
      fprintf(stderr,"make_utf8_string: unexpected UFT-8 character\n");
      exit(1);
    }
  }
  *word = '\0';  // mark the end
}

static void similar_words(hash_table_t *hash_table,hash_table_node_t *from)
{
  static const int valid_characters[] =
  { // unicode!
    0x2D,                                                                       // -
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,           // A B C D E F G H I J K L M
    0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,           // N O P Q R S T U V W X Y Z
    0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,           // a b c d e f g h i j k l m
    0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,           // n o p q r s t u v w x y z
    0xC1,0xC2,0xC9,0xCD,0xD3,0xDA,                                              // Á Â É Í Ó Ú
    0xE0,0xE1,0xE2,0xE3,0xE7,0xE8,0xE9,0xEA,0xED,0xEE,0xF3,0xF4,0xF5,0xFA,0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
    0
  };
  int i,j,k,individual_characters[_max_word_size_];
  char new_word[2 * _max_word_size_];

  break_utf8_string(from->word,individual_characters);
  for(i = 0;individual_characters[i] != 0;i++)
  {
    k = individual_characters[i];
    for(j = 0;valid_characters[j] != 0;j++)
    {
      individual_characters[i] = valid_characters[j];
      make_utf8_string(individual_characters,new_word);
      // avoid duplicate cases
      if(strcmp(new_word,from->word) > 0)
        add_edge(hash_table,from,new_word);
    }
    individual_characters[i] = k;
  }
}


//
// breadth-first search (Has an error that we couldn't solve)
//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//

static int breadth_first_search(int maximum_number_of_vertices, hash_table_node_t **list_of_vertices, hash_table_node_t *origin, hash_table_node_t *goal)
{
  hash_table_node_t *node; 
  adjacency_node_t *neighbor;
  int current = 0, end = 0; // Head and tail of the list_of_vertices
  list_of_vertices[0] = origin;
  origin->visited = 1;

  while (current <= end)
  {
    // Get the current vertex and move to the next for the following iteration
    node = list_of_vertices[current++];
    //printf("passei aqui %d vezes\n",current);

    // Check if we have reached the goal
    if (node == goal)  break;

    // Mark the current vertex as visited
    node->visited = 1;

    // Visit the neighbors of the current vertex
    for (neighbor = node->head; neighbor; neighbor = neighbor->next)
    {
      if (!neighbor->vertex->visited)
      {
        // Add the unvisited neighbor to the list of vertices
        list_of_vertices[++end] = neighbor->vertex;
        neighbor->vertex->visited = 1;
        neighbor->vertex->previous = node;
      }
    }
  }

  // Reset visited vertices for future searches
  for (int i = 0; i < maximum_number_of_vertices; i++)  list_of_vertices[i]->visited = 0;

  if (node == goal)
  {
    return end + 1; // Return the number of visited vertices
  }
  else
  {
    return -1; // Return -1 if the goal was not reached
  }
}



//
// list all vertices belonging to a connected component (Not working due to bfs error somewhere)
//

static void list_connected_component(hash_table_t *hash_table,const char *word)
{
  hash_table_node_t *node = find_word(hash_table, word, 0);

  //Verify that the word exists
  if (node == NULL)
  {
    fprintf(stderr,"list_connected_component: word %s does not exist\n",word);
    return;
  }

  //Get the size of the connected component
  hash_table_node_t *representative = find_representative(node);
  int size = representative->number_of_vertices;

  //Search all words belonging to the connected component
  hash_table_node_t **list_of_vertices = malloc(size * sizeof(hash_table_node_t*));
  int number_of_vertices = breadth_first_search(size, list_of_vertices, node, NULL);

  //List connected component
  for (int i = 0; i < number_of_vertices; i++)
    printf("[ %d] %s\n",i,list_of_vertices[i]->word);
    
  free(list_of_vertices);
}


//
// find the shortest path from a given word to another given word (Only works with smaller text file)
//

static void path_finder(hash_table_t *hash_table,const char *from_word,const char *to_word)
{
  hash_table_node_t *from, *to, *node, *from_representative, *to_representative;

  from = find_word(hash_table, from_word, 0);
  to = find_word(hash_table, to_word, 0);

  //Verify that both words exist 
  if (from == NULL)
  {  
    fprintf(stderr,"path_finder: word %s does not exist\n",from_word);
    return;
  }
  if (to == NULL)
  {
    fprintf(stderr,"path_finder: word %s does not exist\n",to_word);
    return;
  }

  // Verify both words belong to the same connected component
  from_representative = find_representative(from);
  to_representative = find_representative(to);
  if (to_representative != from_representative)
  {
    fprintf(stderr,"path_finder: No path was found from %s to %s\n", from_word, to_word);
    return;
  }

  //Get the size of the connected component and search a path between both words 
  int size = from_representative->number_of_vertices;
  hash_table_node_t **list_of_vertices = malloc(size * sizeof(hash_table_node_t*));
  int visited_vertices = breadth_first_search(size, list_of_vertices, to, from);

  //List the path found
  if (visited_vertices > 0)
  {
    node = list_of_vertices[0];
    printf("Shortest path from %s to %s:\n", to_word, from_word);
    printf("  %s\n", to_word);
    for (int i = 1; i < visited_vertices; i++)
    {
      if (node == list_of_vertices[i]->previous)
      {
        printf("  %s\n", list_of_vertices[i]->word);
        node = list_of_vertices[i];
      }
    }
  }
  else  fprintf(stderr,"path_finder: No path was found from %s to %s\n", from_word, to_word);

  free(list_of_vertices);
}


//
// some graph information
//

static void graph_info(hash_table_t *hash_table)
{
   fprintf(stderr,"\nHash table data:\nSize: %u \nWords inserted: %u\nEdges created: %u\n",
            hash_table->hash_table_size,
            hash_table->number_of_entries,
            hash_table->number_of_edges);
}


//
// main program
//

int main(int argc,char **argv)
{
  char word[100],from[100],to[100];
  hash_table_t *hash_table;
  hash_table_node_t *node;
  unsigned int i;
  int command;
  FILE *fp;

  // initialize hash table
  hash_table = hash_table_create();
  // read words
  fp = fopen((argc < 2) ? "wordlist-big-latest.txt" : argv[1],"rb");
  if(fp == NULL)
  {
    fprintf(stderr,"main: unable to open the words file\n");
    exit(1);
  }
  while(fscanf(fp,"%99s",word) == 1)
    (void)find_word(hash_table,word,1);
  fclose(fp);

  // find all similar words
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next)
      similar_words(hash_table,node);  

  graph_info(hash_table);
  // ask what to do
  for(;;)
  {
    fprintf(stderr,"\nYour wish is my command:\n");
    fprintf(stderr,"  1 WORD       (list the connected component WORD belongs to)\n");
    fprintf(stderr,"  2 FROM TO    (list the shortest path from FROM to TO)\n");
    fprintf(stderr,"  3            (terminate)\n");
    fprintf(stderr,"> ");
    if(scanf("%99s",word) != 1)
      break;
    command = atoi(word);
    if(command == 1)
    {
      if(scanf("%99s",word) != 1)
        break;
      list_connected_component(hash_table,word);
    }
    else if(command == 2)
    {
      if(scanf("%99s",from) != 1)
        break;
      if(scanf("%99s",to) != 1)
        break;
      path_finder(hash_table,from,to);
    }
    else if(command == 3)
      break;
  }
  // clean up
  hash_table_free(hash_table);
  return 0;
}
