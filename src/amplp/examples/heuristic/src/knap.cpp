#include "knap.hpp"

void KnapHeuristic::set_capacity(int capacity){

	this->capacity = capacity;
};


void KnapHeuristic::set_nitems(int nitems){
	this->nitems = nitems;

	weights.resize(nitems, 0);
	values.resize(nitems, 0);
	sol.resize(nitems, 0);
	obj = 0;
};


void KnapHeuristic::set_value(int val, int pos){
	values[pos] = val;
};


void KnapHeuristic::set_weight(int val, int pos){
	weights[pos] = val;
};


int KnapHeuristic::get_sol(int pos){
	return sol[pos];
};


int KnapHeuristic::get_obj(){
	return obj;
};


void KnapHeuristic::solve(){

	std::cout << "Solve:" << std::endl;

	std::clock_t c_start = std::clock();
	std::clock_t c_end = std::clock();
	double time_elapsed = 0;

	obj = 0;

	std::vector<int> indx(nitems); // index of each item
	std::vector<double> vw(nitems); // value per unit of weight (v[i]/w[i])

	for(int i = 0; i < nitems; i++){
		indx[i] = i;
		vw[i] = (double)values[i]/(double)weights[i];
	}

	//~ std::cout << "vw = [";
	//~ for(int i = 0; i < nitems; i++){
		//~ std::cout << vw[i];
		//~ if (i<nitems-1){
			//~ std::cout << ", ";
		//~ }
	//~ }
	//~ std::cout << "]" << std::endl;

	c_end = std::clock();
	time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	std::cout << "Proc " << time_elapsed << std::endl;

	//sort indx by value per unit of weight
	std::sort(indx.begin(), indx.end(), [&](std::size_t a, std::size_t b) { return vw[a] > vw[b]; });


	c_end = std::clock();
	time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	std::cout << "sorted " << time_elapsed << std::endl;



	//~ std::cout << "sorted = [";
	//~ for(int i = 0; i < nitems; i++){
		//~ std::cout << indx[i];
		//~ if (i<nitems-1){
			//~ std::cout << ", ";
		//~ }
	//~ }
	//~ std::cout << "]" << std::endl;

	// add items (if possible) by value per unit of weight
	int used_capacity = 0;

	for(int i = 0; i < nitems; i++){

		int item = indx[i];

		if(used_capacity + weights[item] <= capacity){
			sol[item] = 1;
			obj += values[item];
			used_capacity += weights[item];
		}
		else{
			sol[item] = 0;
		}
	}
	c_end = std::clock();
	time_elapsed = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
	std::cout << "KnapHeuristic done with objective " << obj <<  " in " << time_elapsed << std::endl;
};


void KnapHeuristic::show(){

	std::cout << "KnapHeuristic show:" << std::endl;
	std::cout << "nitems: "<< nitems << std::endl;
	std::cout << "capacity: "<< capacity << std::endl;

	std::cout << "weights: [";
	for (int i=0; i<weights.size(); i++){

		std::cout << weights[i];

		if (i<weights.size()-1){
			std::cout << ", ";
		}
	}
	std::cout << "]" << std::endl;

	std::cout << "values: [";
	for (int i=0; i<values.size(); i++){

		std::cout << values[i];
		if (i<values.size()-1){
			std::cout << ", ";
		}
	}
	std::cout << "]" << std::endl;

	std::cout << "sol: [";
	for (int i=0; i<values.size(); i++){

		std::cout << sol[i];
		if (i<sol.size()-1){
			std::cout << ", ";
		}
	}
	std::cout << "]" << std::endl;


	std::cout << "obj: "<< obj << std::endl;
};



void KnapHeuristic::load(char *path){

	int temp;

	try{
		FILE* data = fopen(path, "r");

		fscanf(data, "%d", &temp);
		set_nitems(temp);

		fscanf(data, "%d", &temp);
		set_capacity(temp);

		for(int i=0; i<nitems; i++){

			fscanf(data, "%d", &temp);
			values[i] = temp;

			fscanf(data, "%d", &temp);
			weights[i] = temp;
		}
	}
	catch(...)
	{
		std::cout << "Cannot load data"<< obj << std::endl;
	}
};








