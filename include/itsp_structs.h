/*
 * itsp_structs.h
 *
 *  Created on: 27 f�vr. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_STRUCTS_H_
#define INCLUDE_ITSP_STRUCTS_H_

extern int MODE_FRANCAIS;

#define HEADER_SIZE 31
#define CHECKSUM_SIZE 5

#include <i_tools.h>

typedef enum itsp_data_type{
	data_link			= 'L',
	data_config			= 'C',
	data_race_status	= 'S',
	data_pools			= 'P',
	data_scan			= 'X',
	data_pool_total		= 'T',
	data_payoffs		= '$',
	data_results		= 'R',
	data_alert			= 'A',
	data_will_pay		= 'W',
	data_file_transfert	= 'F'
} itsp_data_type;

typedef enum itsp_msg_type{
	msg_pending			= 'P',
	msg_request			= 'R',
	msg_data			= 'D',
	msg_acknowledge		= 'A'
} itsp_msg_type;

typedef enum itsp_msg_reason{
	reason_fault				=   0,
	reason_ok					=   1,
	reason_no_reason 			= ' ',
	reason_begin 				= 'b',
	reason_end 					= 'e',
	reason_final 				= 'f',
	reason_host 				= 'h',
	reason_remote 				= 'r',
	reason_invalid_header 		= 'H',
	reason_unhandled_data		= 'U',
	reason_invalid_data			= 'J',
	reason_data_checksum 		= 'D',
	reason_invalid_race 		= 'R',
	reason_invalid_pool 		= 'P',
	reason_race_closed 			= 'C',
	reason_bad_total 			= 'S',
	reason_invalid_source_code	= 'V',
	reason_inappropriate 		= 'I',
	reason_not_avalable 		= 'A',
	reason_invalid_event 		= 'T',
	reason_terminate_sequence	= 'X',
	reason_formate_error 		= '?'
}itsp_msg_reason;

typedef enum pool_status_type{
	Open_pool 		= 'O',
	Closed_pool 	= 'C',
	Cancelled_pool 	= 'X',
	Exchange_pool 	= 'E',
	Payed_pool		= 'P'
}pool_status_type;

typedef enum race_status_type{
	Open_race 		= 'O',
	Closed_race 	= 'C',
	Cancelled_race 	= 'X',
	Post_Time_race 	= 'P',
	Official_race 	= 'F',
	Unofficial_race = 'U'
}race_status_type;

typedef enum scan_mode_type{
	s_mod_pool 				= 'P',
	s_mod_early 			= 'E',
	s_mod_combination_early = 'K',
	s_mod_quiniela 			= 'Q',
	s_mod_alternate_runner 	= 'A',
	s_mod_late 				= 'L',
	s_mod_superfecta 		= 'S',
	s_mod_exact_n 			= 'X',
	s_mod_combination_late	= 'C'
}scan_mode_type;

typedef enum when_value_type{
	when_never		= 'N',
	when_final		= 'F',
	when_all_cycles	= 'A'
}when_value_type;

typedef enum comm_round_type{
	com_round_up	= 'U',
	com_round_down	= 'D',
	com_round_true	= 'R'
}comm_round_type;

typedef enum itsp_data_config_mode{
	itsp_dc_mode_host = 'H',
	itsp_dc_mode_remote = 'R'
}itsp_dt_config_mode;

typedef struct itsp_frame_type{
	itsp_data_type 	data;												/* Code du type de donnees                         */
	itsp_msg_type 	message; 											/* Code du type de message                         */
	itsp_msg_reason reason;		                                        /* Code du champ 'reason'                          */
}itsp_frame_type, *p_itsp_frame_type;

typedef struct itsp_str_time {
    int hour ;
    int minutes ;
    int seconds ;
} itsp_str_time, *p_itsp_str_time ;

typedef struct itsp_str_date {
    int year ;
    int month ;
    int day ;
} itsp_str_date, *p_itsp_str_date ;

typedef enum itsp_pool_mode{
	gross_pool_mode	= 'G',
	net_pool_mode	= 'N'
}itsp_pool_mode;

typedef enum itsp_pool_calculation{
	standard_pool_calculation	= 'S',
	net_pool_calculation		= 'N'
}itsp_pool_calculation;

#define ITSP_MAX_LENGTH_SOURCE_NAME 3
#define ITSP_MAX_LENGTH_EVENT_CODE_NAME 3
#define ITSP_MAX_LENGTH_POOL_CODE 3
#define TOLERANCE 0.00001

#define set_fr_type(type_, data_, msg_, reason_) ({\
		type_.data = data_;\
		type_.message = msg_;\
		type_.reason = reason_;\
 })

/********************************************************************************************/
/*                                                                                          */
/*                                           HEADER                                         */
/*                                                                                          */
/********************************************************************************************/
typedef struct itsp_header{
	char*				header_log;
	itsp_frame_type 	frame_type[1];
    char            	source    [ITSP_MAX_LENGTH_SOURCE_NAME + 1] ;     /* Valeur du champ 'source' : sur 3 caracteres     */
    int             	sequence ;                                        /* Numero de sequence                              */
    char            	event_code[ITSP_MAX_LENGTH_EVENT_CODE_NAME + 1] ; /* Valeur du champ 'event_code' : sur 4 caracteres */
    int             	race_number ;                                     /* Numero de l'epreuve                             */
    char            	pool_code [ITSP_MAX_LENGTH_POOL_CODE + 1] ;       /* Pool code                                       */
    itsp_str_time		time ;                                            /* Heure                                           */
    int             	ldata ;                                           /* Longueur des donnees suivant le 'Header'        */
} itsp_header, *p_itsp_header ;
/********************************************************************************************/

/********************************************************************************************/
/*                                                                                          */
/*                                           DATA                                           */
/*                                                                                          */
/********************************************************************************************/
typedef struct itsp_data_struct{
	char*				data_log;
	p_itsp_frame_type 	frame_type;
	p_void 			  	structure;
}itsp_data_struct, *p_itsp_data_struct;
/********************************************************************************************/



/********************************************************************************************/
/*                                                                                          */
/*                                           FRAME                                          */
/*                                                                                          */
/********************************************************************************************/
typedef struct itsp_frame{
	p_itsp_header 		header;
	p_itsp_data_struct	data_str;
	int					frame_num;
	int					host_flag;
	long				date_time;
}itsp_frame, *p_itsp_frame;
/********************************************************************************************/


#define MAX_SEQ_FRAMES 5
/********************************************************************************************/
/*                                                                                          */
/*                                          SEQUENCE                                        */
/*                                                                                          */
/********************************************************************************************/
typedef enum itsp_sequence_type{
	error_sequence			=-1,
	short_pending_sequence 	= 0,
	short_request_sequence 	= 1,
	pending_sequence		= 2,
	request_sequence 		= 3,
	data_sequence			= 4
}itsp_sequence_type;

typedef struct itsp_sequence{
	itsp_data_type		data_type;
	itsp_sequence_type	seq_type;
	char				from[4];
	char				to[4];
	char				event[4];
	int 				seq_num;
	int 				nb_frames;
	int					incomplete_flag;
	p_itsp_frame		frames[MAX_SEQ_FRAMES];
}itsp_sequence, *p_itsp_sequence;
/********************************************************************************************/


#define ITSP_MAX_LENGTH_DOCUMENT_NAME 4
/********************************************************************************************/
/*                                                                                          */
/*                                           LINK                                           */
/*                                                                                          */
/********************************************************************************************/
typedef struct itsp_str_version                                /* <version>                 */
{                                                              /*****************************/
    int   version_number ;                                     /* <version number>          */
    int   revision_number ;                                    /* <revision number>         */
} itsp_str_version ;                                           /*****************************/
typedef struct itsp_str_identifier                             /* <identifier>              */
{                                                              /*****************************/
    char          document[ITSP_MAX_LENGTH_DOCUMENT_NAME + 1] ;/* <document>                */
    itsp_str_version version ;                                 /* <version>                 */
} itsp_str_identifier ;                                        /*                           */
/********************************************************************************************/
typedef struct itsp_str_data_link                              /* <link>                    */
{                                                              /*****************************/
    itsp_str_identifier identifier ;                           /* <identifier>              */
    char*               text ;								   /* [<text>]                  */
} itsp_str_data_link, *p_itsp_str_data_link ;                  /*****************************/
/********************************************************************************************/

#define ITSP_NBMAX_RACES 30
#define ITSP_NBMAX_COM_RATE 10
#define ITSP_NBMAX_POOLS 30
/********************************************************************************************/
/*                                                                                          */
/*                                       CONFIGURATION                                      */
/*                                                                                          */
/********************************************************************************************/
typedef struct itsp_str_race_list                              /* <race list>               */ /* COMMUNS */
{                                                              /*****************************/
    int   races ;                                              /* <races>                   */
    int   race[ITSP_NBMAX_RACES] ;                             /* {<race>}*r                */
} itsp_str_race_list ;                                         /*                           */
                                                               /*****************************/
typedef struct itsp_str_commission_rate                        /* <commision_rate>          */
{                                                              /*****************************/
    double 	rate ;                                             /* <rate>                    */
    char    rounding ;                                         /* <rounding>                */
} itsp_str_commission_rate ;                                   /*                           */
                                                               /*****************************/
typedef struct itsp_str_pool_config		                       /* <pool configuration>      */
{                                                              /*****************************/
    char                     pool_code[4] ;                    /* <pool code>               */
    char                     exchange_part ;                   /* <exchange part>           */
    scan_mode_type           scan_mode ;                       /* <scan mode>               */
    when_value_type          send_gross_pool ;                 /* <send gross pool>         */
    when_value_type          receive_gross_pool ;              /* <receive gross pool>      */
    when_value_type          send_net_pool ;                   /* <send net pool>           */
    when_value_type          receive_net_pool ;                /* <receive net pool>        */
    when_value_type          send_total ;                      /* <send total>              */
    when_value_type          receive_total ;                   /* <receive total>           */
    when_value_type          xfer_will_pay ;                   /* <xfer will pay>           */
    double                   pool_unit ;                       /* <pool unit>               */
    double                   min_payoff ;                      /* <min payoff>              */
    double                   Break ;                           /* <break>                   */
    int                      nb_commissions ;                  /* <commissions>             */
    itsp_str_commission_rate commissions[ITSP_NBMAX_COM_RATE] ;/* (c*<commision rate>)      */
} itsp_str_pool_config, *p_itsp_str_pool_config ;              /*                           */
                                                               /*                           */
/********************************************************************************************/
typedef struct   itsp_str_data_config                          /* <configuration>           */
{                                                              /*****************************/
	itsp_dt_config_mode  	mode ;                             /* <mode>                    */
	itsp_pool_calculation	calculation ;                      /* <calculation>             */
    itsp_str_date        	date ;                             /* <date>                    */
    itsp_str_time       	time ;                             /* <time>                    */
    int                  	performance ;                      /* <performance>             */
    int                  	close_bet_delay ;                  /* <close bet delay>         */
    int                  	close_cancel_delay ;               /* <close cancel delai>      */
    itsp_str_race_list   	race_list ;                        /* <race list>               */
    int                  	nb_pools ;                         /* <pools>                   */
    itsp_str_pool_config 	pool_configs[ITSP_NBMAX_POOLS] ;   /* {<pool configuration>}*p  */
} itsp_str_data_config, *p_itsp_str_data_config ;              /*                           */
/********************************************************************************************/


#define ITSP_NBMAX_RUNNERS 30
#define ITSP_NBMAX_BRACKETS 20
/********************************************************************************************/
/*                                                                                          */
/*                                   DATA RACE STATUS                                       */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
	int*						race_list ;
	char						pool_code[4] ;
    pool_status_type			pool_status ;
    double						min_bet ;
    double						net_addin ;
    double						gross_addin ;
} itsp_str_pool_definition, *p_itsp_str_pool_definition ;

typedef struct
{
    char						pool_code[4] ;
    int							race ;
	int**						pool_runners;
} itsp_str_scratch_info, *p_itsp_str_scratch_info ;

typedef struct
{
	race_status_type			status ;
    itsp_str_time				post_time ;
    char						display ;
    int							runners ;
    char						runner_status [ITSP_NBMAX_RUNNERS] ;
    int							brackets ;
    int*						bracket_status[ITSP_NBMAX_BRACKETS] ;
    int							pools ;
    itsp_str_pool_definition	pool_def      [ITSP_NBMAX_POOLS] ;
    int							scratch_pools ;
    itsp_str_scratch_info		scratch_info  [ITSP_NBMAX_POOLS] ;
} itsp_str_data_race_status, *p_itsp_str_data_race_status ;
/********************************************************************************************/

#define NBMAX_ROWS_POOL_DATA      30
#define NBMAX_COLUMNS_POOL_DATA   30
/********************************************************************************************/
/*                                                                                          */
/*                                       DATA POOLS                                         */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
	itsp_pool_mode	pool_mode ;			//G -> Gross, N -> Net
    int   			segments ;
    int  			segment ;
    int  			leg;
} itsp_str_pool_header ;

typedef struct
{
    int                       rows ;
    int                       columns ;
    double**				  matrix_data ;
    double                    segment_total ;
    double                    total ;
    double                    net_total ;
} itsp_str_pool_data ;

typedef struct
{
    itsp_str_pool_header   pool_header ;
    itsp_str_pool_data     pool_data ;
} itsp_str_data_pools, *p_itsp_str_data_pools ;
/********************************************************************************************/


/********************************************************************************************/
/*                                                                                          */
/*                                       DATA TOTALS                                        */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
    double live_total ;
    double net_total ;
} itsp_str_data_totals, *p_itsp_str_data_totals ;
/********************************************************************************************/


/********************************************************************************************/
/*                                                                                          */
/*                                	 SCAN REQUEST BEGIN                                     */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
    char 	scan_mode ;
    int 	combos ;
    int** 	scan_runners;
    int** 	live_runners;
    int** 	favorite_runners;
    int** 	sub_runners;
    int 	leg;
} itsp_str_scan_header, *p_itsp_str_scan_header ;
/********************************************************************************************/


/********************************************************************************************/
/*                                                                                          */
/*                                	 	  DATA SCAN                                     	*/
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
	p_itsp_str_scan_header 	scan_header ;
	p_itsp_str_data_pools	scan_data ;
	double					scan_total;
	double					live_total;
} itsp_str_data_scan, *p_itsp_str_data_scan ;
/********************************************************************************************/

typedef enum payoff_price_status{
	price_refund			= 'R',
	price_exchange			= 'E',
	price_normal			= '$',
	price_pay_exchange		= 'P',
	price_alternate			= 'A',
	price_itq_consolation	= 'C',
	price_itq_pay_exch		= 'Q',
	price_partial_refund	= 'r'
}payoff_price_status;
/********************************************************************************************/
/*                                                                                          */
/*                                       DATA PAYOFFS                                       */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
	payoff_price_status 	status ;
    int 					winners ;
    char					order_flag ;
    int**					runner_list ;
    double 					price;
    double					winning;
    double					liability;
    double					break_amount;
} itsp_price_definition, *p_itsp_price_definition;

typedef struct
{
    int** 					live_runners;
    double 					refund ;
    double 					carry_over;
    int 					nb_commissions;
    double* 				commissions;
    int 					nb_prices;
    p_itsp_price_definition price_definition;
} itsp_str_data_payoffs, *p_itsp_str_data_payoffs;


/********************************************************************************************/


/********************************************************************************************/
/*                                                                                          */
/*                                       DATA RESULTS                                       */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
    int 	runner ;
    char 	entry ;
    int		position ;
} itsp_finish, *p_itsp_finish;

typedef struct
{
    int 					nb_finshers ;
    itsp_finish*			finishs ;
    int* 					favorite_list;
} itsp_str_data_results, *p_itsp_str_data_results;
/********************************************************************************************/


/********************************************************************************************/
/*                                                                                          */
/*                                       DATA Alert                                         */
/*                                                                                          */
/********************************************************************************************/
#define DATA_ALERT_MAX_MSG_LENGTH 256
typedef struct
{
    char 	type ;
    char*	message ;
} itsp_str_alert, *p_itsp_str_alert;

/********************************************************************************************/


/********************************************************************************************/
/*                                                                                          */
/*                                       DATA Will Pay                                      */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
    int 	winner ;
    double 	price;
    double	winning;
} itsp_will_pay_item, *p_itsp_will_pay_item;

typedef struct
{
    int**				 runner_list ;
    itsp_will_pay_item   *items;
} itsp_will_pay_row, *p_itsp_will_pay_row;

typedef struct
{
	int     				rows;
	int     				columns;
	p_itsp_will_pay_row 	will_pay_row;
} itsp_str_will_pay, *p_itsp_str_will_pay;

/********************************************************************************************/


/********************************************************************************************/
/*                                                                                          */
/*                                       DATA Xfile	                                        */
/*                                                                                          */
/********************************************************************************************/
typedef struct
{
    char 			destination[4];
    char 			name[33];
    itsp_str_date	date;
    itsp_str_time	time;
    int 			size;
    int 			segments;
    int 			current_segment;
    int 			segment_size;
} itsp_file_header, *p_itsp_file_header;

typedef struct
{
	itsp_file_header		file_header;
	char*     				file_data;
} itsp_str_file, *p_itsp_str_file;

/********************************************************************************************/

#endif /* INCLUDE_ITSP_STRUCTS_H_ */
