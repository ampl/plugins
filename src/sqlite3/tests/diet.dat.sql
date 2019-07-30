begin;
create table nutrients(
	nutr varchar primary key not null,
	n_min integer not null,
	n_max integer not null
) without rowid;
insert into nutrients(nutr, n_min, n_max) values
		('A', 700, 20000),
		('C', 700, 20000),
		('B1', 700, 20000),
		('B2', 700, 20000),
		('NA', 0, 50000),
		('CAL', 16000, 24000);

create table foods(
	food varchar primary key not null,
	cost real not null,
	f_min integer not null,
	f_max integer not null
) without rowid;
insert into foods(food, cost, f_min, f_max) values 
		('BEEF', 3.19, 2, 10),
		('CHK', 2.59, 2, 10),
		('FISH', 2.29, 2, 10),
		('HAM', 2.89, 2, 10),
		('MCH', 1.89, 2, 10),
		('MTL', 1.99, 2, 10),
		('SPG', 1.99, 2, 10),
		('TUR', 2.49, 2, 10);

create table food_nutr_amt(
	food varchar not null references foods(food),
	nutr varchar not null references nutrients(nutr),
	amt integer not null,
	primary key(food, nutr)
) without rowid;
insert into food_nutr_amt(food, nutr, amt) values
		('BEEF', 'A', 60),
		('BEEF', 'C', 20),
		('BEEF', 'B1', 10),
		('BEEF', 'B2', 15),
		('BEEF', 'NA', 938),
		('BEEF', 'CAL', 295),
		('CHK', 'A', 8),
		('CHK', 'C', 0),
		('CHK', 'B1', 20),
		('CHK', 'B2', 20),
		('CHK', 'NA', 2180),
		('CHK', 'CAL', 770),
		('FISH', 'A', 8),
		('FISH', 'C', 10),
		('FISH', 'B1', 15),
		('FISH', 'B2', 10),
		('FISH', 'NA', 945),
		('FISH', 'CAL', 440),
		('HAM', 'A', 40),
		('HAM', 'C', 40),
		('HAM', 'B1', 35),
		('HAM', 'B2', 10),
		('HAM', 'NA', 278),
		('HAM', 'CAL', 430),
		('MCH', 'A', 15),
		('MCH', 'C', 35),
		('MCH', 'B1', 15),
		('MCH', 'B2', 15),
		('MCH', 'NA', 1182),
		('MCH', 'CAL', 315),
		('MTL', 'A', 70),
		('MTL', 'C', 30),
		('MTL', 'B1', 15),
		('MTL', 'B2', 15),
		('MTL', 'NA', 896),
		('MTL', 'CAL', 400),
		('SPG', 'A', 25),
		('SPG', 'C', 50),
		('SPG', 'B1', 25),
		('SPG', 'B2', 15),
		('SPG', 'NA', 1329),
		('SPG', 'CAL', 370),
		('TUR', 'A', 60),
		('TUR', 'C', 20),
		('TUR', 'B1', 15),
		('TUR', 'B2', 10),
		('TUR', 'NA', 1397),
		('TUR', 'CAL', 450);
commit;
--testing
/*
--.header on
select * from set_FOOD;
select * from set_NUTR;
select * from param_cost;
select * from param_f_min;
select * from param_f_max;
select * from param_n_min;
select * from param_n_max;
select * from param_amt;
*/
