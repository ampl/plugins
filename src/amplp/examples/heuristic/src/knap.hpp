#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
#include <ctime>


/* Basic heuristic to solve 0-1 Knapsack problems
 */
class KnapHeuristic{
public:

int nitems = 0; // number of items
int capacity = 0; // maximum capacity of the knapsack
int obj = 0; // result of the heuristic

std::vector<int> weights; // weight of each item
std::vector<int> values; // value of each item
std::vector<int> sol; // 1 if item i is in the knapsack, 0 otherwise

//KnapHeuristic(){};
//~KnapHeuristic();

/* Sets the number of possible items to choose from and allocates the
 * corresponding space in the auxiliary vectors
 */
void set_nitems(int nitems);

/* Sets the maximum capacity of the knapsack
 */
void set_capacity(int capacity);

/* Set the value of a given item
 */
void set_value(int val, int pos);

/* Set the weight of a given item
 */
void set_weight(int val, int pos);

/* Get the solution value of a given item
 */
int get_sol(int pos);

/* Get the objective of the heuristic
 */
int get_obj();

/* Prints all the information
 */
void show();

/* Execute the heuristic
 */
void solve();

/* Load data from a text file
 */
void load(char *path);
};
