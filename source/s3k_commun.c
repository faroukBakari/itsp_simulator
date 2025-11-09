/*
 * s3k_commun.c
 *
 *  Created on: 9 oct. 2017
 *      Author: f.baccari
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <math.h>

 #include <i_string.h>
 #include <i_file.h>
 #include <s3k_structs.h>
 #include <s3k_session.h>
 #include <itsp_common.h>
 #include <itsp_cnx.h>

int** itsp_make_live_runner(p_s_s3k_pool_type p_pool_typ, p_s_s3k_pool p_pool){
	int **ptr = NULL, *live_runners[p_pool_typ->dimensions + 1];

	if(p_pool_typ->dimensions != p_pool_typ->nb_races){
		if(p_pool_typ->dimensions % p_pool_typ->nb_races){
			DEBUG_WARN("Can't handle dimensions case for pool \"%s\"", p_pool->code);
			abort();
		}

		int diplications = p_pool_typ->dimensions / p_pool_typ->nb_races;

		ptr = live_runners;
		live_runners[p_pool_typ->dimensions] = NULL;
		while(diplications--){
			memcpy(ptr, p_pool->scratch_info, p_pool_typ->nb_races * sizeof(int*));
			ptr += p_pool_typ->nb_races;
		}
		ptr = live_runners;
	}
	else
		ptr = p_pool->scratch_info;

	return clone_combo(ptr);
}

int** itsp_make_favorite_runners(p_s_s3k_grep p_grep, p_s_s3k_pool_type p_pool_typ, p_s_s3k_pool p_pool, int leg){

	int i = p_pool_typ->nb_races, *fav[i + 1];
	fav[i] = NULL;
	int zero = 0;

	while(i--){
		p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &p_pool->race_list[i]);
		if(!p_race_i){
			DEBUG_WARN("race %d not found in grep \"%s\"", i, p_grep->name);
			abort();
		}
		fav[i] = p_race_i->favorite_list;
	}

	int **ptr = NULL, *favorite_runners[p_pool_typ->dimensions + 1];
	favorite_runners[p_pool_typ->dimensions] = NULL;

	if(p_pool_typ->dimensions != p_pool_typ->nb_races){
		if(p_pool_typ->dimensions % p_pool_typ->nb_races){
			DEBUG_WARN("Can't handle dimensions case for pool \"%s\"", p_pool->code);
			abort();
		}
		i = p_pool_typ->dimensions / p_pool_typ->nb_races;
		ptr = favorite_runners;
		while(i--){
			memcpy(ptr, fav, p_pool_typ->nb_races * sizeof(int*));
			ptr += p_pool_typ->nb_races;
		}
		ptr = favorite_runners;
	}
	else
		ptr = fav;

	for(i = leg - 1; i < p_pool_typ->dimensions; i++)
		ptr[i] = &zero;

	return clone_combo(ptr);
}

int** itsp_make_scan_runners(p_s_s3k_grep p_grep, p_s_s3k_pool_type p_pool_typ, p_s_s3k_pool p_pool, int leg){

	int i = 0;
	//int combo[p_pool_typ->dimensions + 1][3];
	int** combo = malloc((p_pool_typ->dimensions + 1) * sizeof(int*));
	combo[p_pool_typ->dimensions] = NULL;
	while(i < p_pool_typ->dimensions){
		combo[i] = malloc(3 * sizeof(int));
		memset(combo[i++], 0, 3 * sizeof(int));
	}

	if(leg < 1 || p_pool_typ->nb_races < leg){
		DEBUG_WARN("pool \"%s\" -> leg %d incoherence with nb_races (%d)", p_pool_typ->code, leg, p_pool_typ->nb_races);
		abort();
	}

	i = 0;
	if(p_pool_typ->code[0] == 'P')
		while(i < leg - 1){
			p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &p_pool->race_list[i]);
			if(!p_race_i){
				DEBUG_WARN("race %d not found in grep \"%s\"", i, p_grep->name);
				abort();
			}

			if(!fast_map_count(p_race_i->finish->finishers)){
				DEBUG_WARN("race %d of grep \"%s\" -> no finish entries found!", i, p_grep->name);
				abort();
			}
			int pos = 1;
			combo[i++][0] = ((p_itsp_finish)fast_map_at(p_race_i->finish->finishers, &pos))->runner;

		}

	int **out = clone_combo((int**)combo);
	return out;
}




