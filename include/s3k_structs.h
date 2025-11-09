/*
 * s3k_structs.h
 *
 *  Created on: 13 avr. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_S3K_STRUCTS_H_
#define INCLUDE_S3K_STRUCTS_H_

#include <i_fast_map.h>
#include <i_sorted_vect.h>
#include <i_sorted_set.h>
#include <i_fifo.h>
#include <itsp_structs.h>

typedef enum ret_code{
	err_pools		= -12,
	err_abo_pool	= -11,
	err_abo_race	= -10,
	err_abo			= -9,
	err_protocol	= -8,
	err_config		= -7,
	err_null_param 	= -6,
	err_type 		= -5,
	err_att 		= -4,
	err_grep 		= -3,
	err_race 		= -2,
	err_pool 		= -1,
	err_default 	=  0,
	ret_ok 			=  1
}ret_code;

typedef struct s_s3k_attrib{
	int 				id_att;
	char 				code_att[4];
	char* 				att_description;
	itsp_str_identifier	itsp_version;
	long				jet_lag;
}s_s3k_attrib, *p_s_s3k_attrib;

typedef struct s_s3k_pool_type{
	int 	id_pg;
	char	code[4];
	int		nb_races;
	int		dimensions;
}s_s3k_pool_type, *p_s_s3k_pool_type;

typedef struct s_s3k_att_conf{
	char						att[4];
	char* 						host_adr_ip;
	char* 						remote_adr_ip;
	p_s_fast_map				abo_races;				//int, s_s3k_abo_race
	p_s_fast_map				pool_configs;			//pool_code, itsp_str_pool_config
}s_s3k_att_conf, *p_s_s3k_att_conf;

typedef struct s_s3k_grep{
	int 					nu_grep;
	long					date_time;
	itsp_pool_calculation	calculation;
	int 					close_bet_delay;
	int 					close_cancel_delay;
	int                  	performance;
	char 					name[4];
	char					org_att[4];
	unsigned short int		port;
	p_s_fast_map			races;						//int, s_s3k_race
	p_s_fast_map			default_pool_configs;		//pool_code, itsp_str_pool_config
	p_s_fast_map			col_configs;				//att_code, s_s3k_att_conf
}s_s3k_grep, *p_s_s3k_grep;

typedef struct s_s3k_abo_race{
	int						number;
	p_s_sorted_set			abo_pools;					//pool_code
}s_s3k_abo_race, *p_s_s3k_abo_race;

typedef struct s_s3k_result{
	p_s_sorted_set	sent_atts;
	p_s_sorted_set	sent_offic_atts;
	int				offic_flag;
	long			finish_time;
	p_s_fast_map	finishers;							//int, itsp_finish
}s_s3k_result, *p_s_s3k_result;

typedef struct s_s3k_race{
	int					number;
	char				display;
	p_s_fast_map		runners;						//int, s_s3k_runner
	int*				favorite_list;
	p_s_sorted_vect		brackets;						//s_s3k_bracket
	p_s_fast_map		pools;							//pool_code, s_s3k_pool
	race_status_type	status;
	time_t				start;
	p_s_s3k_result		finish;
}s_s3k_race, *p_s_s3k_race;

typedef struct s_s3k_pool{
	int*				race_list;						//int
	int*				dimensions;
	int**				scratch_info;
	int**				sub_runners;
	char				code[4];
	pool_status_type 	status;
    double				min_bet;
    double				net_addin;
    double				gross_addin;
    double				jackpot;
    p_s_fast_map		bets;							//att_code, s_s3k_bets
    p_s_fast_map		payoffs;						//att_code, p_s_fast_map(win_combo, s_s3k_payoffs)
    p_s_fast_map		will_pay;						//att_code, p_s_fast_map(win_combo, p_itsp_will_pay_row)
}s_s3k_pool, *p_s_s3k_pool;

typedef struct s_s3k_combo{
	int			cardinal;
	int**		list;
}s_s3k_combo, *p_s_s3k_combo;

typedef struct s_s3k_payoffs{
	char				att[4];
	int					sent_flag;					//host_only flag
	payoff_price_status	status;
	int**				win_combo;
	int					least_combo_match;
	double				win_coef;
	double				Eligible_mass;			// a revoir pour les trammes payoff -> winning
	double				total_winning;			// a revoir pour les trammes payoff -> liability
	double				rounding_rest;			// a revoir pour les trammes payoff -> break_amount
	char				order_flag;
}s_s3k_payoffs, *p_s_s3k_payoffs;

typedef struct s_s3k_bets{
	int					vtf_flag;						//host_only flag
	scan_mode_type		exchange_style;
	itsp_pool_mode		amounts_type;
	double*				amounts;
	p_s_fast_map		scans;							//int(leg), s_s3k_bet_scan
	double				total;
	double				refund_total;
	double				net_total;
}s_s3k_bets, *p_s_s3k_bets;

typedef struct s_s3k_bet_scan{
	int 				leg;
	int**				combo;
	int					rows;
	int					columns;
	double*				amounts;
	double				scan_total;
	double				live_total;
}s_s3k_bet_scan, *p_s_s3k_bet_scan;

typedef struct s_s3k_bracket{
	int					id;
	int*				runner_list;
}s_s3k_bracket, *p_s_s3k_bracket;

typedef struct s_s3k_runner{
	int					number;
	int					bracket;
	char 				status;
}s_s3k_runner, *p_s_s3k_runner;


/*typedef enum s3k_event_type{
	event_parti			= 1,
	event_arrivee_prov	= 2,
	event_arrivee_def 	= 3,
	event_scratch		= 4
}s3k_event_type;

typedef struct s_s3k_event{
	char			grep[4];
	int				race;
	long			date_time;
	s3k_event_type	type;
	p_void			data;
}s_s3k_event, *p_s_s3k_event;*/


#endif /* INCLUDE_S3K_STRUCTS_H_ */
